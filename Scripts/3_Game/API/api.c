/*
 * GameLabs core web api
 *
 * REST style is not available due to header limitations
 */

class _Callback_RegisterAsync : _Callback {
    override void OnError(int errorCode) {
        GetGameLabs().GetLogger().Error(string.Format("RegisterAsync errorCode(%1)", errorCode));
        GetGameLabs().GetApi().Disable();
    };

    override void OnTimeout() {
        GetGameLabs().GetLogger().Error(string.Format("RegisterAsync timed out"));
        GetGameLabs().GetApi().Disable();
    };

    override void OnSuccess(string data, int dataSize) {
        _Response_Register response = new _Response_Register(data);
        GetGameLabs().GetLogger().Debug(string.Format("ServerPoll OnSuccess(%1) : %2", response, data));
        if(response.status == 2) {
            GetGameLabs().GetLogger().Warn(string.Format("RegisterAsync re-authorized"));
            GetGameLabs()._PropagateFeatures(response); // Access features outside of API class
            GetGameLabs().GetApi().SetAuthkey(response.authKey);
            GetGameLabs().GetApi().Enable();
        } else {
            GetGameLabs().GetLogger().Warn(string.Format("RegisterAsync auth failed"));
            GetGameLabs().GetApi().Disable();
        }
    };
};

class GameLabsAPI {
    private string baseUrl = "https://api.cftools.cloud/gamelabs/dz";
    private string storeUrl = "https://api.cftools.cloud/gamelabs/dz";

    private RestApi restApi;
    private RestContext restContext;
    private ref JsonSerializer jsonSerializer = new JsonSerializer();

    private string serverId;
    private string apiKey;
    private string authKey;

    private bool active = false;

    void GameLabsAPI(string serverId, string apiKey, string baseUrl = "https://api.cftools.cloud/gamelabs/dz", string storeUrl = "https://api.cftools.cloud/gamelabs/dz") {
        this.serverId = serverId;
        this.apiKey = apiKey;

        if(baseUrl) this.baseUrl = baseUrl;
        if(storeUrl) this.storeUrl = storeUrl;

        this.restApi = CreateRestApi();
        this.restContext = this.restApi.GetRestContext(this.baseUrl);

        this.restApi.EnableDebug(false);
    }

    void Enable() { this.active = true; }
    void Disable() { this.active = false; }
    bool IsEnabled() { return this.active; }

    void SetAuthkey(string authKey) {
        this.authKey = authKey;
        this.restContext.SetHeader(authKey); // Currently setting to Content-Type; DZ does not allow to set custom headers
        GetGameLabs().GetLogger().Debug(string.Format("authKey=%1", authKey));
    }

    int Register() {
        this.restContext.SetHeader("application/json"); // Expected for auth request

        _Payload_Register payload = new _Payload_Register(this.serverId, this.apiKey);
        _Response_Register response = new _Response_Register(this.restContext.POST_now("/v1/auth/register", payload.ToJson()));
        if(response.status == 2) {
            GetGameLabs()._PropagateFeatures(response); // Access features outside of API class

            this.SetAuthkey(response.authKey);
        }
        return response.status;
    }

    void RegisterAsync() {
        this.restContext.SetHeader("application/json"); // Expected for auth request
        _Payload_Register payload = new _Payload_Register(this.serverId, this.apiKey);
        this.restContext.POST(new _Callback_RegisterAsync(), "/v1/auth/register", payload.ToJson());
    }

    int Verify() {
        if(!this.IsEnabled()) return -1;

        _Payload payload = new _Payload();
        _Response response = new _Response(this.restContext.POST_now("/v1/auth/verify", payload.ToJson()));
        return response.status;
    }

    void ServerPoll(Managed cb, _Payload_ServerPoll payload) {
        if(!this.IsEnabled()) return;
        this.restContext.POST(RestCallback.Cast(cb), "/v1/server/poll", payload.ToJson());
    }

    void ServerEvents(Managed cb, _Payload_ServerEvents payload) {
        if(!this.IsEnabled()) return;
        this.restContext.POST(RestCallback.Cast(cb), "/v1/server/events", payload.ToJson());
    }

    void ServerVehicles(Managed cb, _Payload_ServerVehicles payload) {
        if(!this.IsEnabled()) return;
        this.restContext.POST(RestCallback.Cast(cb), "/v1/server/vehicles", payload.ToJson());
    }

    void ServerPlayers(Managed cb, _Payload_ServerPlayers payload) {
        if(!this.IsEnabled()) return;
        this.restContext.POST(RestCallback.Cast(cb), "/v1/server/players", payload.ToJson());
    }

    void PlayerConnect(Managed cb, _Payload_PlayerConnect payload) {
        if(!this.IsEnabled()) return;
        this.restContext.POST(RestCallback.Cast(cb), "/v1/player/connect", payload.ToJson());
    }

    void PlayerDeath(Managed cb, _Payload_PlayerDeath payload) {
        if(!this.IsEnabled()) return;
        this.restContext.POST(RestCallback.Cast(cb), "/v1/player/death", payload.ToJson());
    }

    void PlayerDamage(Managed cb, _Payload_PlayerDamage payload) {
        if(!this.IsEnabled()) return;
        this.restContext.POST(RestCallback.Cast(cb), "/v1/player/damage", payload.ToJson());
    }

    void PlayerChat(Managed cb, _Payload_PlayerChat payload) {
        if(!this.IsEnabled()) return;
        this.restContext.POST(RestCallback.Cast(cb), "/v1/player/chat", payload.ToJson());
    }

    void ItemInteract(Managed cb, _Payload_ItemInteract payload) {
        if(!this.IsEnabled()) return;
        this.restContext.POST(RestCallback.Cast(cb), "/v1/item/interact", payload.ToJson());
    }

    void ItemPlace(Managed cb, _Payload_ItemPlace payload) {
        if(!this.IsEnabled()) return;
        this.restContext.POST(RestCallback.Cast(cb), "/v1/item/place", payload.ToJson());
    }
};