modded class TerritoryFlag extends BaseBuildingBase {
    ref _Event _registeredInstance;

    void TerritoryFlag () {
        // This defers initialization of the GameLabs _Event as it probably breaks execution of other modded
        // constructors because it uses a hard ref to the instance
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Call(this._InitGameLabs);
	}

    private void _InitGameLabs() {
        this._registeredInstance = new _Event("TerritoryFlag", "pennant", this);

        if(GetGameLabs()) {
            if(GetGameLabs().IsServer()) {
                GetGameLabs().RegisterEvent(this._registeredInstance);
            }
        }
    }

    void ~TerritoryFlag () {
        if(GetGameLabs()) {
            if(GetGameLabs().IsServer()) {
                if(this._registeredInstance) GetGameLabs().RemoveEvent(this._registeredInstance);
            }
        }
    }

    // Public API
    _Event _GetEventInstance() {
        return this._registeredInstance;
    }

    void _SetEventInstance(_Event _registeredInstance) {
        this._registeredInstance = _registeredInstance;
    }
};