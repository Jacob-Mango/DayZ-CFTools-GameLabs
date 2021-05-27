modded class ZombieBase  {
    private ref _AI _registeredInstance = new _AI(this, true);

    void ZombieBase () {
        if(!GetGameLabs()) return;
        if(!GetGameLabs().IsServer()) return;
        GetGameLabs().IncrAICount();
        GetGameLabs().RegisterAI(this._registeredInstance);
    }

    void ~ZombieBase () {
        if(!GetGameLabs()) return;
        if(!GetGameLabs().IsServer()) return;
        GetGameLabs().DecrAICount();
        if(this._registeredInstance) GetGameLabs().RemoveAI(this._registeredInstance);
    }
};