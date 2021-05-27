modded class PlayerBase extends ManBase {
    private string gl_steam64 = "";
    private string gl_name = "Survivor";

    private string gl_cftoolsId = "";

    private int gl_speedHackTriggers = 0;
    private float gl_ticktime = 0.0;
    private vector gl_position;

    string GetPlainId() {
        return this.gl_steam64;
    }

    string GetPlayerName() {
        return this.gl_name;
    }

    void SetUpstreamIdentity(string cftoolsId) {
        this.gl_cftoolsId = cftoolsId;
    }
    string GetUpstreamIdentity() {
        return this.gl_cftoolsId;
    }
    bool HasUpstreamIdentity() {
        return (this.gl_cftoolsId == "");
    }

    // This is supposed to be overriden by third party mods instead of hooking into the workflow directly
    void OnUpstreamIdentityReceived() {}

    void OnGameLabsSync() {
        if(GetGameLabs().GetDebugStatus()) {
            this.ShowWeaponDebug(true);
        }
    }

    void GameLabs_OnConnect(string steam64, string name) {
        GetGameLabs().GetLogger().Debug(string.Format("GameLabs_OnConnect(this=%1, steam64=%2, name=%3)", this, steam64, name));
        this.gl_steam64 = string.Format("%1", steam64);
        this.gl_name = string.Format("%1", name);

        this.gl_position = this.GetPosition();
        this.gl_position[1] = 0; // Work with 2D only
    }

    void GameLabs_MakeReady(string steam64, string name) {
        GetGameLabs().GetLogger().Debug(string.Format("GameLabs_MakeReady(this=%1, steam64=%2, name=%3)", this, steam64, name));
        this.gl_steam64 = string.Format("%1", steam64);
        this.gl_name = string.Format("%1", name);
    }

    override void OnScheduledTick(float deltaTime) {
        super.OnScheduledTick(deltaTime);
        float tickTime = GetGame().GetTickTime();
        float diff = (tickTime - this.gl_ticktime);
        if(diff >= GetGameLabs().GetConfiguration().GetPlayerTickInterval()) {
            #ifdef GAMELABSVERBOSE
            GetGameLabs().GetLogger().Debug(string.Format("OnScheduledTick(this=%1, deltaTime=%2, diff=%3, tickTime=%4)", this, deltaTime, diff, tickTime));
            #endif
            this.gl_ticktime = tickTime;
            this._Tick(diff, tickTime);
        }
    }

    void _Tick(float diff, float tickTime) {
        vector currentPosition = this.GetPosition();
        currentPosition[1] = 0; // Work with 2D only
        float distance = vector.Distance(this.gl_position, currentPosition);
        float unitsPerSecond = distance / diff;
        this.gl_position = currentPosition;

        #ifdef GAMELABSVERBOSE
        GetGameLabs().GetLogger().Debug(string.Format("_Tick(this=%1, distance=%2, unitsPerSecond=%3)", this, distance, unitsPerSecond));
        #endif

        if(GetGameLabs().GetConfiguration().GetSpeedCheckStatus()) {
            float warningThreshold;
            if (this.IsInVehicle()) {
                warningThreshold = GetGameLabs().GetConfiguration().GetSpeedCheckThresholdVehicle();
            } else {
                warningThreshold = GetGameLabs().GetConfiguration().GetSpeedCheckThresholdFoot();
            }

            if (unitsPerSecond >= warningThreshold) {
                this.gl_speedHackTriggers++;
                GetGameLabs().GetLogger().Warn("********* EXPERIMENTAL FEATURE *********");
                GetGameLabs().GetLogger().Warn(string.Format("[SPEED-HACK] Potential speed-hack player=%1, distance=%2u, unitsPerSecond=%3 [threshold=%4, inVehicle=%5, triggers=%6])", this.GetPlainId(), distance, unitsPerSecond, warningThreshold, this.IsInVehicle(),this.gl_speedHackTriggers));
                GetGameLabs().GetLogger().Warn("****************************************");
            }
            if (this.gl_speedHackTriggers > 2) {
                // TODO: Invoke remote action
            }
        }
    }

    void OverridePosition(vector position) {
        this.gl_position = position;
    }

    void SetPositionEx(vector position) {
        this.OverridePosition(position);
        this.SetPosition(position);
    }

    override void OnVehicleSwitchSeat(int seatIndex) {
        super.OnVehicleSwitchSeat(seatIndex);
        GetGameLabs().GetLogger().Debug(string.Format("OnVehicleSwitchSeat(this=%1, seatIndex=%2)", this, seatIndex));
    }

    override void EEKilled(Object killer) {
        super.EEKilled(killer);
        if(!GetGame().IsServer()) return;
        if(!GetGameLabs().IsStatReportingEnabled()) return;

        ref _Payload_PlayerDeath payload;
        ref _LogPlayerEx logObjectMurderer;
        ref _LogPlayerEx logObjectPlayer = new _LogPlayerEx(this);

        EntityAI weapon;

        PlayerBase murderer;
        if(killer.IsWeapon() || killer.IsMeleeWeapon()) {
            weapon = EntityAI.Cast(killer);
            murderer = PlayerBase.Cast(weapon.GetHierarchyParent());
        }
        GetGameLabs().GetLogger().Debug(string.Format("EEKilled(this=%1, killer=%2, weapon=%3, murderer=%4)", this, killer, weapon, murderer));

        if(murderer) {
            logObjectMurderer = new _LogPlayerEx(murderer);
            payload = new _Payload_PlayerDeath(logObjectPlayer, logObjectMurderer, killer.GetType(), killer.GetDisplayName());
        } else if(this == killer) { // Suicide, potentially Environmental death
            if(weapon) {
                payload = new _Payload_PlayerDeath(logObjectPlayer, NULL, killer.GetType(), killer.GetDisplayName());
            } else if(this.CommitedSuicide() || this.CommitedSuicide()) {
                weapon = this.GetItemInHands();
                if(weapon) payload = new _Payload_PlayerDeath(logObjectPlayer, NULL, "__Suicide", weapon.GetType());
                else payload = new _Payload_PlayerDeath(logObjectPlayer, NULL, "__Suicide", "");
            }
            else payload = new _Payload_PlayerDeath(logObjectPlayer, NULL, "__Environment", "");

        } else { // Infected
            payload = new _Payload_PlayerDeath(logObjectPlayer, NULL, "__Infected", "");
        }

        GetGameLabs().GetApi().PlayerDeath(new _Callback(), payload);
    }

    override void EEHitBy(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef) {
        super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
        if(!GetGame().IsServer()) return;
        if(!GetGameLabs().IsStatReportingEnabled()) return;

        DayZPlayerImplement playerImplement = DayZPlayerImplement.Cast(this);
        GetGameLabs().GetLogger().Debug(string.Format("EEHitBy.EVAL (deathSync=%1, suicide=%2)", this.m_DeathSyncSent, this.CommitedSuicide()));
        if(this.m_DeathSyncSent || this.CommitedSuicide()) return; // Prevent logging of hits for dead bodies

        ref _Payload_PlayerDamage payload;
        ref _LogPlayerEx logObjectMurderer;
        ref _LogPlayerEx logObjectPlayer = new _LogPlayerEx(this);

        PlayerBase murderer;
        if(source) {
            murderer = PlayerBase.Cast(source.GetHierarchyParent());
        }
        if(!source || !murderer) return;

        GetGameLabs().GetLogger().Debug(string.Format("EEHitBy(this=%1, murderer=%2, source=%3, component=%4, dmgZone=%5, ammo=%6, modelPos=%7, speedCoef=%8)", this, murderer, source, component, dmgZone, ammo, modelPos, speedCoef));
        logObjectMurderer = new _LogPlayerEx(murderer);
        payload = new _Payload_PlayerDamage(logObjectPlayer, logObjectMurderer, source, damageResult.GetDamage(dmgZone, "Health"), dmgZone);
        GetGameLabs().GetApi().PlayerDamage(new _Callback(), payload);
    }
};