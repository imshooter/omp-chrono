#include "chrono.hpp"

using namespace chrono;

class ChronoComponent final : public IComponent, public PawnEventHandler {
public:
	PROVIDE_UID(0x6368726f6e6f0001);

	StringView componentName() const override {
		return "Chrono";
	}

	SemanticVersion componentVersion() const override {
		return SemanticVersion(COMPONENT_VERSION_MAJOR, COMPONENT_VERSION_MINOR, COMPONENT_VERSION_PATCH, 0);
	}

	void provideConfiguration(ILogger& logger, IEarlyConfig& config, bool defaults) override {
		if (defaults || config.getType("chrono.time_zone") == ConfigOptionType_None) {
			config.setString("chrono.time_zone", "");
		}
	}

	void onLoad(ICore* core) override {
		core_ = core;
	}

	void onInit(IComponentList* components) override {
		pawn_ = components->queryComponent<IPawnComponent>();
		attach(pawn_);

		if (pawn_ != nullptr) {
			pawn_->getEventDispatcher().addEventHandler(this);
		}

		load(*core_, core_->getConfig().getString("chrono.time_zone"));
	}

	void onAmxLoad(IPawnScript& script) override {
		registerNatives(script);
	}

	void onAmxUnload(IPawnScript& script) override {
	}

	void onFree(IComponent* component) override {
		if (component == pawn_ && pawn_ != nullptr) {
			pawn_->getEventDispatcher().removeEventHandler(this);
			pawn_ = nullptr;
			attach(nullptr);
		}
	}

	void reset() override {
	}

	void free() override {
		if (pawn_ != nullptr) {
			pawn_->getEventDispatcher().removeEventHandler(this);
		}

		delete this;
	}

private:
	ICore* core_ = nullptr;
	IPawnComponent* pawn_ = nullptr;
};

COMPONENT_ENTRY_POINT() {
	return new ChronoComponent();
}
