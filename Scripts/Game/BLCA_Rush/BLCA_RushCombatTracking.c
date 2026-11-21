modded class SCR_CharacterDamageManagerComponent
{
 protected static const float BLCA_COMBAT_LOCK_MS = 8000;
 protected float m_BLCACombatUntil;
 protected EventHandlerManagerComponent m_BLCAFireEvents;

 static SCR_CharacterDamageManagerComponent BLCA_RushCombatManager(IEntity character)
 {
  if (!character) return null;
  return SCR_CharacterDamageManagerComponent.Cast(character.FindComponent(SCR_CharacterDamageManagerComponent));
 }

 float BLCA_RushCombatUntil()
 {
  return m_BLCACombatUntil;
 }

 void BLCA_TrackRushShots()
 {
  if (m_BLCAFireEvents || !Replication.IsServer()) return;
  m_BLCAFireEvents = EventHandlerManagerComponent.Cast(GetOwner().FindComponent(EventHandlerManagerComponent));
  if (m_BLCAFireEvents) m_BLCAFireEvents.RegisterScriptHandler("OnProjectileShot", this, BLCA_OnRushShot, false);
 }

 protected void BLCA_OnRushShot(int playerId, BaseWeaponComponent weapon, IEntity projectile)
 {
  if (!Replication.IsServer()) return;
  BLCA_RushGameModeComponent mode = BLCA_RushGameModeComponent.Current();
  if (mode && mode.Phase() == BLCA_RushPhase.ROUND_ACTIVE)
   m_BLCACombatUntil = BLCA_RushGameModeComponent.Now() + BLCA_COMBAT_LOCK_MS;
 }

 override bool HijackDamageHandling(notnull BaseDamageContext damageContext)
 {
  if (BLCA_RushCombatBoundary.BlocksDamage(GetOwner(), damageContext)) return true;
  return super.HijackDamageHandling(damageContext);
 }

 protected override void OnDamage(notnull BaseDamageContext damageContext)
 {
  super.OnDamage(damageContext);
  if (!Replication.IsServer() || damageContext.damageValue <= 0 || !damageContext.instigator) return;
  BLCA_RushGameModeComponent mode = BLCA_RushGameModeComponent.Current();
  if (!mode || mode.Phase() != BLCA_RushPhase.ROUND_ACTIVE) return;

  int victimTeam = mode.TeamOfEntity(GetOwner());
  IEntity attacker = damageContext.instigator.GetInstigatorEntity();
  int attackerTeam = mode.TeamOfEntity(attacker);
  int attackerId = damageContext.instigator.GetInstigatorPlayerID();
  if (attackerId > 0)
  {
   attackerTeam = mode.TeamOfPlayer(attackerId);
   attacker = GetGame().GetPlayerManager().GetPlayerControlledEntity(attackerId);
  }
  if (victimTeam < 0 || attackerTeam < 0 || victimTeam == attackerTeam) return;

  m_BLCACombatUntil = BLCA_RushGameModeComponent.Now() + BLCA_COMBAT_LOCK_MS;
  SCR_CharacterDamageManagerComponent attackerDamage = BLCA_RushCombatManager(attacker);
  if (attackerDamage) attackerDamage.m_BLCACombatUntil = m_BLCACombatUntil;
 }

 override void OnDelete(IEntity owner)
 {
  if (m_BLCAFireEvents)
   m_BLCAFireEvents.RemoveScriptHandler("OnProjectileShot", this, BLCA_OnRushShot, false);
  super.OnDelete(owner);
 }
}

modded class SCR_DamageManagerComponent
{
 override bool HijackDamageHandling(notnull BaseDamageContext damageContext)
 {
  if (BLCA_RushCombatBoundary.BlocksDamage(GetOwner(), damageContext)) return true;
  return super.HijackDamageHandling(damageContext);
 }
}

