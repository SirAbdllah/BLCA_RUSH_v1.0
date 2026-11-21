class BLCA_RushVoice
{
 static const ResourceName PUBLIC_ITEM = "{44B3E655E2CF5369}Prefabs/BLCA_Rush/BLCA_VoicePublic.et";
 static const ResourceName SQUAD_ITEM = "{F4CD71AA6B5E58E1}Prefabs/BLCA_Rush/BLCA_VoiceSquad.et";
 static const int PUBLIC_FREQUENCY = 62000;
 static const int SQUAD_FREQUENCY = 62200;
 static const float RANGE = 65535;
 static const string PUBLIC_KEY = "BLCARush/Public";

 static int Frequency(IEntity item)
 {
  if (!item || !item.GetPrefabData()) return 0;
  ResourceName prefab = item.GetPrefabData().GetPrefabName();
  if (prefab == PUBLIC_ITEM) return PUBLIC_FREQUENCY;
  if (prefab == SQUAD_ITEM) return SQUAD_FREQUENCY;
  return 0;
 }

 static string SquadKey(int playerId)
 {
  if (playerId <= 0) return "";
  SCR_GroupsManagerComponent groups = SCR_GroupsManagerComponent.GetInstance();
  if (!groups) return "";
  SCR_AIGroup squad = groups.GetPlayerGroup(playerId);
  if (!squad || !squad.GetFaction() || !squad.IsPlayerInGroup(playerId) || squad.GetGroupID() < 0) return "";
  string factionKey = squad.GetFaction().GetFactionKey();
  if ((factionKey != "US" && factionKey != "USSR")) return "";
  return "BLCARush/Squad/" + factionKey + "/" + squad.GetGroupID();
 }

 static string ChannelName(int frequency)
 {
  if (frequency == PUBLIC_FREQUENCY) return "Public";
  return "Private Squad";
 }

 static BaseRadioComponent Radio(IEntity item)
 {
  if (!item) return null;
  return BaseRadioComponent.Cast(item.FindComponent(BaseRadioComponent));
 }

 static void PowerDown(IEntity item)
 {
  BaseRadioComponent radio = Radio(item);
  if (radio && radio.IsPowered()) radio.SetPower(false);
 }
}

class BLCA_RushVoiceInventory
{
 IEntity Body;
 IEntity PublicRadio;
 IEntity SquadRadio;
}

modded class BLCA_RushGameModeComponent
{
 protected ref map<int, ref BLCA_RushVoiceInventory> m_BLCA_Voice = new map<int, ref BLCA_RushVoiceInventory>();

 override void OnGameModeStart()
 {
  super.OnGameModeStart();
  if (!GetGameMode().IsMaster()) return;
  SCR_AIGroup.GetOnPlayerAdded().Insert(BLCA_OnVoiceSquadChanged);
  SCR_AIGroup.GetOnPlayerRemoved().Insert(BLCA_OnVoiceSquadChanged);
  GetGame().GetCallqueue().CallLater(BLCA_ReconcileVoice, 500, true);
 }

 override void OnGameModeEnd(SCR_GameModeEndData data)
 {
  BLCA_StopVoice();
  super.OnGameModeEnd(data);
 }

 override void OnDelete(IEntity owner)
 {
  BLCA_StopVoice();
  super.OnDelete(owner);
 }

 override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
 {
  BLCA_ClearVoice(playerId);
  super.OnPlayerDisconnected(playerId, cause, timeout);
 }

 protected void BLCA_StopVoice()
 {
  SCR_AIGroup.GetOnPlayerAdded().Remove(BLCA_OnVoiceSquadChanged);
  SCR_AIGroup.GetOnPlayerRemoved().Remove(BLCA_OnVoiceSquadChanged);
  if (GetGame()) GetGame().GetCallqueue().Remove(BLCA_ReconcileVoice);
  array<int> ids = {};
  foreach (int playerId, BLCA_RushVoiceInventory channels : m_BLCA_Voice) ids.Insert(playerId);
  foreach (int playerId : ids) BLCA_ClearVoice(playerId);
 }

 protected void BLCA_OnVoiceSquadChanged(SCR_AIGroup squad, int playerId)
 {
  if (!GetGameMode().IsMaster()) return;
  BLCA_RushVoiceInventory channels;
  if (!m_BLCA_Voice.Find(playerId, channels) || !channels) return;
  // Revoke old reception as soon as membership changes. The next pass uses
  // the settled group roster, after the native join/leave handlers finish.
  BLCA_RushVoice.PowerDown(channels.SquadRadio);
 }

 protected void BLCA_ClearVoice(int playerId)
 {
  BLCA_RushVoiceInventory channels;
  if (!m_BLCA_Voice.Find(playerId, channels) || !channels) return;
  BLCA_DeleteVoiceRadio(channels.Body, channels.PublicRadio);
  BLCA_DeleteVoiceRadio(channels.Body, channels.SquadRadio);
  m_BLCA_Voice.Remove(playerId);
 }

 protected void BLCA_DeleteVoiceRadio(IEntity body, IEntity item)
 {
  if (!item || item.IsDeleted()) return;
  BLCA_RushVoice.PowerDown(item);
  InventoryStorageManagerComponent inventory;
  if (body && !body.IsDeleted()) inventory = InventoryStorageManagerComponent.Cast(body.FindComponent(InventoryStorageManagerComponent));
  if (!inventory || !inventory.Contains(item) || !inventory.TryDeleteItem(item))
   SCR_EntityHelper.DeleteEntityAndChildren(item);
 }

 protected void BLCA_ReconcileVoice()
 {
  if (!GetGameMode().IsMaster()) return;
  PlayerManager players = GetGame().GetPlayerManager();
  if (!players) return;
  array<int> ids = {};
  players.GetPlayers(ids);
  foreach (int playerId : ids)
  {
   ChimeraCharacter body = ChimeraCharacter.Cast(players.GetPlayerControlledEntity(playerId));
   if (TeamOfPlayer(playerId) < 0 || !body || !body.GetCharacterController() || body.GetCharacterController().IsDead())
   {
    BLCA_ClearVoice(playerId);
    continue;
   }
   InventoryStorageManagerComponent inventory = InventoryStorageManagerComponent.Cast(body.FindComponent(InventoryStorageManagerComponent));
   if (!inventory) continue;
   BLCA_RushVoiceInventory channels;
   if (m_BLCA_Voice.Find(playerId, channels) && channels && channels.Body != body)
   {
    BLCA_ClearVoice(playerId);
    channels = null;
   }
   if (!channels)
   {
    channels = new BLCA_RushVoiceInventory();
    channels.Body = body;
    m_BLCA_Voice.Set(playerId, channels);
   }
   channels.PublicRadio = BLCA_EnsureVoiceRadio(body, inventory, channels.PublicRadio, BLCA_RushVoice.PUBLIC_ITEM, BLCA_RushVoice.PUBLIC_KEY);
   string squadKey = BLCA_RushVoice.SquadKey(playerId);
   if (!squadKey.IsEmpty())
    channels.SquadRadio = BLCA_EnsureVoiceRadio(body, inventory, channels.SquadRadio, BLCA_RushVoice.SQUAD_ITEM, squadKey);
   else if (channels.SquadRadio)
   {
    BLCA_DeleteVoiceRadio(body, channels.SquadRadio);
    channels.SquadRadio = null;
   }
  }
 }

 protected IEntity BLCA_EnsureVoiceRadio(IEntity body, InventoryStorageManagerComponent inventory, IEntity item, ResourceName prefab, string key)
 {
  if (item && (item.IsDeleted() || !inventory.Contains(item)))
  {
   BLCA_DeleteVoiceRadio(body, item);
   item = null;
  }
  bool created = !item;
  if (created)
  {
   Resource resource = Resource.Load(prefab);
   if (!resource || !resource.IsValid()) return null;
   EntitySpawnParams params = new EntitySpawnParams();
   params.TransformMode = ETransformMode.WORLD;
   params.Transform[3] = body.GetOrigin();
   item = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
  }
  BaseRadioComponent radio = BLCA_RushVoice.Radio(item);
  BaseTransceiver channel;
  if (radio) channel = radio.GetTransceiver(0);
  if (!channel)
  {
   BLCA_DeleteVoiceRadio(body, item);
   return null;
  }
  if (radio.GetEncryptionKey() != key)
  {
   BLCA_RushVoice.PowerDown(item);
   radio.SetEncryptionKey(key);
  }
  int frequency = BLCA_RushVoice.Frequency(item);
  if (channel.GetFrequency() != frequency) channel.SetFrequency(frequency);
  if (channel.GetRange() != BLCA_RushVoice.RANGE) channel.SetRange(BLCA_RushVoice.RANGE);
  if (created && !inventory.TryInsertItem(item, EStoragePurpose.PURPOSE_DEPOSIT))
  {
   BLCA_DeleteVoiceRadio(body, item);
   return null;
  }
  if (!radio.IsPowered()) radio.SetPower(true);
  return item;
 }

 IEntity BLCA_VoiceRadio(int playerId, bool squad)
 {
  BLCA_RushVoiceInventory channels;
  if (!m_BLCA_Voice.Find(playerId, channels) || !channels) return null;
  if (squad) return channels.SquadRadio;
  return channels.PublicRadio;
 }
}

modded class SCR_VONEntryRadio
{
 protected int BLCA_ManagedFrequency()
 {
  if (!BLCA_RushGameModeComponent.Current() || !m_RadioTransceiver) return 0;
  BaseRadioComponent radio = m_RadioTransceiver.GetRadio();
  if (!radio) return 0;
  return BLCA_RushVoice.Frequency(radio.GetOwner());
 }

 override void SetChannelText(string channel)
 {
  int frequency = BLCA_ManagedFrequency();
  if (frequency) channel = BLCA_RushVoice.ChannelName(frequency);
  super.SetChannelText(channel);
 }

 override void AdjustEntry(int modifier)
 {
  if (!BLCA_ManagedFrequency()) super.AdjustEntry(modifier);
 }

 override void AdjustEntryModif(int modifier)
 {
  if (!modifier || !BLCA_ManagedFrequency()) super.AdjustEntryModif(modifier);
 }

 override void ToggleEntry()
 {
  if (!BLCA_ManagedFrequency()) super.ToggleEntry();
 }
}

modded class SCR_VONController
{
 protected string m_BLCA_ApprovedVoiceKey;

 override protected void InitEncryptionKey()
 {
  super.InitEncryptionKey();
  if (!m_BLCA_ApprovedVoiceKey.IsEmpty()) m_sLocalEncryptionKey = m_BLCA_ApprovedVoiceKey;
 }

 override protected bool ActivateVON(notnull SCR_VONEntry entry, EVONTransmitType transmitType = EVONTransmitType.NONE)
 {
  SCR_VONEntryRadio radioEntry = SCR_VONEntryRadio.Cast(entry);
  if (!BLCA_RushGameModeComponent.Current() || !radioEntry || !radioEntry.GetTransceiver()) return super.ActivateVON(entry, transmitType);
  BaseRadioComponent radio = radioEntry.GetTransceiver().GetRadio();
  if (!radio) return super.ActivateVON(entry, transmitType);
  IEntity item = radio.GetOwner();
  int frequency = BLCA_RushVoice.Frequency(item);
  if (!frequency) return super.ActivateVON(entry, transmitType);
  PlayerController player = GetGame().GetPlayerController();
  if (!player || player != GetOwner()) return false;
  IEntity body = player.GetControlledEntity();
  if (!BLCA_RushGameModeComponent.CanInteract(body)) return false;
  InventoryStorageManagerComponent inventory = InventoryStorageManagerComponent.Cast(body.FindComponent(InventoryStorageManagerComponent));
  if (!inventory || !inventory.Contains(item) || !radio.IsPowered() || radioEntry.GetTransceiver().GetFrequency() != frequency) return false;

  string key = BLCA_RushVoice.PUBLIC_KEY;
  if (frequency == BLCA_RushVoice.SQUAD_FREQUENCY) key = BLCA_RushVoice.SquadKey(player.GetPlayerId());
  if (key.IsEmpty() || radio.GetEncryptionKey() != key) return false;

  string oldKey = m_sLocalEncryptionKey;
  m_BLCA_ApprovedVoiceKey = key;
  bool activated = super.ActivateVON(entry, transmitType);
  m_BLCA_ApprovedVoiceKey = "";
  m_sLocalEncryptionKey = oldKey;
  return activated;
 }
}
