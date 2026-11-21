class BLCA_RushResultView
{
 static const ResourceName LAYOUT = "{C8DFB3C3E7C45CB7}UI/layouts/BLCA_Rush/BLCA_Result.layout";
 protected static const ResourceName ROW = "{96F5D9C51E745191}UI/layouts/BLCA_Rush/BLCA_ResultRow.layout";
 protected static const int HUD_ROWS_PER_TEAM = 16;
 protected Widget m_Root;
 protected string m_LastSummary;
 protected bool m_Loaded;
 protected int m_LastWinner = -2;
 protected int m_LastTeam = -2;
 protected int m_LastSeconds = -1;
 protected ref array<ref CanvasWidgetCommand> m_Crown = {};
 protected float m_CrownScale;

 void Update(Widget root, BLCA_RushGameModeComponent mode, bool nativeScreen = false)
 {
  if (!root || !mode) return;
  m_Root = root;
  int winner = mode.Winner();
  int localPlayer = SCR_PlayerController.GetLocalPlayerId();
  int localTeam = mode.TeamOfPlayer(localPlayer);
  if (winner != m_LastWinner || localTeam != m_LastTeam)
  {
   m_LastWinner = winner;
   m_LastTeam = localTeam;
   string title = "ROUND COMPLETE";
   if (winner >= 0 && localTeam >= 0)
   {
    title = "DEFEAT";
    if (winner == localTeam) title = "VICTORY";
   }
   SetText(root, "ResultTitle", title);
   string winnerName;
   if (winner == 0) winnerName = "US ARMY WINS";
   if (winner == 1) winnerName = "SOVIET ARMY WINS";
   SetText(root, "ResultWinner", winnerName);
   SetVisible(root, "WinnerAttack", winner == 0);
   SetVisible(root, "WinnerDefend", winner == 1);
  }
  UpdateCrown(winner);

  int seconds = Math.Max(0, mode.PhaseSeconds());
  if (!nativeScreen && seconds != m_LastSeconds)
  {
   m_LastSeconds = seconds;
   string remainder = (seconds % 60).ToString();
   if (seconds % 60 < 10) remainder = "0" + remainder;
   SetText(root, "ResultCountdown", string.Format("RESULTS CLOSE IN %1:%2", seconds / 60, remainder));
  }

  string summary = mode.Summary();
  if (m_Loaded && summary == m_LastSummary) return;
  m_Loaded = true;
  m_LastSummary = summary;
  BLCA_RushScoreboardSnapshot snapshot = new BLCA_RushScoreboardSnapshot();
  if (!summary.IsEmpty())
  {
   JsonLoadContext json = new JsonLoadContext();
   if (!json.LoadFromString(summary) || !json.ReadValue("scoreboard", snapshot) || !snapshot)
    return;
  }
  if (!snapshot.Rows) return;
  PaintTeam("Attack", 0, snapshot.AttackScore, snapshot.AttackCount, snapshot.Rows, localPlayer, nativeScreen);
  PaintTeam("Defend", 1, snapshot.DefendScore, snapshot.DefendCount, snapshot.Rows, localPlayer, nativeScreen);
 }

 protected void UpdateCrown(int winner)
 {
  if (winner < 0) return;
  float scale = GetGame().GetWorkspace().DPIScale(1);
  if (m_CrownScale == scale) return;
  m_CrownScale = scale;
  m_Crown.Clear();
  CrownPolygon({5, 12, 23, 12, 21, 17, 7, 17}, 0xffe7a52f);
  CrownPolygon({3, 5, 10, 12, 5, 12}, 0xffffd274);
  CrownPolygon({9, 12, 14, 3, 19, 12}, 0xffffd274);
  CrownPolygon({18, 12, 25, 5, 23, 12}, 0xffffd274);
  CrownPolygon({7, 18, 21, 18, 21, 20, 7, 20}, 0xffffc355);
  Widget attack = m_Root.FindAnyWidget("WinnerAttack");
  Widget defend = m_Root.FindAnyWidget("WinnerDefend");
  if (!attack || !defend) return;
  CanvasWidget left = CanvasWidget.Cast(attack.FindAnyWidget("Crown"));
  CanvasWidget right = CanvasWidget.Cast(defend.FindAnyWidget("Crown"));
  if (left) left.SetDrawCommands(m_Crown);
  if (right) right.SetDrawCommands(m_Crown);
 }

 protected void CrownPolygon(array<float> points, int color)
 {
  PolygonDrawCommand part = new PolygonDrawCommand();
  part.m_iColor = color;
  part.m_Vertices = {};
  foreach (float point : points) part.m_Vertices.Insert(point * m_CrownScale);
  m_Crown.Insert(part);
 }

 protected void PaintTeam(string side, int team, int score, int total, array<ref BLCA_RushScoreboardRow> rows, int localPlayer, bool nativeScreen)
 {
  Widget list = m_Root.FindAnyWidget(side + "Rows");
  if (!list) return;
  while (list.GetChildren()) list.GetChildren().RemoveFromHierarchy();
  int count;
  foreach (BLCA_RushScoreboardRow row : rows)
  {
   if (!row || row.Team != team) continue;
   count++;
   if (!nativeScreen && count > HUD_ROWS_PER_TEAM) continue;
   Widget entry = GetGame().GetWorkspace().CreateWidgets(ROW, list);
   if (!entry) continue;
   SetText(entry, "Rank", count.ToString());
   SetText(entry, "Name", row.Name);
   SetText(entry, "Score", row.Score.ToString());
   SetText(entry, "Kills", row.Kills.ToString());
   SetText(entry, "Deaths", row.Deaths.ToString());
   SetText(entry, "Plants", row.Plants.ToString());
   SetText(entry, "Defuses", row.Defuses.ToString());

   Widget background = entry.FindAnyWidget("RowBackground");
   if (background && count % 2 == 0) background.SetColorInt(0xff112332);
   if (background && localPlayer > 0 && row.PlayerID == localPlayer)
   {
    background.SetColorInt(0xff1c4861);
    SetVisible(entry, "LocalPlayerRule", true);
   }
  }
  SetText(m_Root, side + "Score", score.ToString());
  int visible = count;
  if (!nativeScreen) visible = Math.Min(visible, HUD_ROWS_PER_TEAM);
  string players = string.Format("%1 PLAYERS", total);
  if (total == 1) players = "1 PLAYER";
  if (visible < total) players = string.Format("TOP %1 OF %2 PLAYERS", visible, total);
  SetText(m_Root, side + "Count", players);
  SetVisible(m_Root, side + "Empty", count == 0);
 }

 protected void SetVisible(Widget root, string name, bool visible)
 {
  Widget widget = root.FindAnyWidget(name);
  if (widget) widget.SetVisible(visible);
 }

 protected void SetText(Widget root, string name, string value)
 {
  TextWidget text = TextWidget.Cast(root.FindAnyWidget(name));
  if (text && text.GetText() != value) text.SetText(value);
 }
}

class BLCA_RushEndResultComponent : SCR_GameOverScreenContentUIComponent
{
 protected ref BLCA_RushResultView m_Result = new BLCA_RushResultView();

 override void InitContent(SCR_GameOverScreenUIContentData endScreenUIContent)
 {
  UpdateResult();
 }

 void UpdateResult()
 {
  m_Result.Update(m_wRoot, BLCA_RushGameModeComponent.Current(), true);
 }
}
