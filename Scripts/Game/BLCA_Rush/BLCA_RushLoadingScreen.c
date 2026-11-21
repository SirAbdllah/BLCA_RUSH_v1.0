modded class SCR_LoadingScreenComponent
{
 protected const ResourceName BLCA_RUSH_LOADING_LAYOUT = "{5AF3009A45C35307}UI/layouts/BLCA_Rush/BLCA_LoadingScreen.layout";
 protected const ResourceName BLCA_RUSH_LOADING_TEXTURE = "{D2A2D7C3746B5B19}Assets/BLCA/BLCA_Logo.edds";
 protected Widget m_BLCALoadingArtwork;
 protected ImageWidget m_BLCARushLoadingLogo;
 protected vector m_BLCAStockLoadingLogoSize;

 override void OnMissionDataRetrieved(SCR_MissionHeader header)
 {
  BLCA_ClearLoadingArtwork();
  super.OnMissionDataRetrieved(header);
  if (!header || header.m_sGameMode != "Rush" || !m_wRoot || header.m_sLoadingScreen != BLCA_RUSH_LOADING_TEXTURE)
   return;

  WorkspaceWidget workspace = m_wRoot.GetWorkspace();
  if (!workspace) return;
  m_BLCALoadingArtwork = workspace.CreateWidgets(BLCA_RUSH_LOADING_LAYOUT, m_wRoot);
  if (!m_BLCALoadingArtwork) return;

  ImageWidget image = ImageWidget.Cast(m_BLCALoadingArtwork.FindAnyWidget("BLCA_LoadingImage"));
  if (!image || !image.LoadImageTexture(0, header.m_sLoadingScreen))
  {
   m_BLCALoadingArtwork.RemoveFromHierarchy();
   m_BLCALoadingArtwork = null;
   return;
  }
  image.SetImage(0);
  m_BLCALoadingArtwork.SetZOrder(-1);
  TextWidget author = TextWidget.Cast(m_BLCALoadingArtwork.FindAnyWidget("BLCA_LoadingAuthor"));
  if (author) author.SetText(header.m_sAuthor);
  BLCA_SetNativeLoadingArtwork(false);
  ImageWidget logo = ImageWidget.Cast(m_wRoot.FindAnyWidget("m_wReforgerLogo"));
  if (logo)
  {
   m_BLCAStockLoadingLogoSize = logo.GetSize();
   if (BLCA_RushBranding.ApplyLogo(logo, 160)) m_BLCARushLoadingLogo = logo;
  }
  if (m_Widgets.m_wPreloadContent) m_Widgets.m_wPreloadContent.SetVisible(false);
 }

 override void OnHideInternal()
 {
  super.OnHideInternal();
  BLCA_ClearLoadingArtwork();
 }

 protected void BLCA_SetNativeLoadingArtwork(bool visible)
 {
  if (!m_wRoot) return;
  Widget background = m_wRoot.FindAnyWidget("m_wBackground");
  Widget photo = m_wRoot.FindAnyWidget("m_wPhotoGroup");
  Widget text = m_wRoot.FindAnyWidget("m_wTextGroup");
  Widget hint = m_wRoot.FindAnyWidget("m_wHintGroup");
  if (background) background.SetVisible(visible);
  if (photo) photo.SetVisible(visible);
  if (text) text.SetVisible(visible);
  if (hint) hint.SetVisible(visible);
 }

 protected void BLCA_ClearLoadingArtwork()
 {
  if (m_BLCARushLoadingLogo)
  {
   m_BLCARushLoadingLogo.SetImage(0);
   m_BLCARushLoadingLogo.SetSize(m_BLCAStockLoadingLogoSize[0], m_BLCAStockLoadingLogoSize[1]);
   m_BLCARushLoadingLogo.Update();
   m_BLCARushLoadingLogo = null;
  }

  if (!m_BLCALoadingArtwork) return;
  m_BLCALoadingArtwork.RemoveFromHierarchy();
  m_BLCALoadingArtwork = null;
  BLCA_SetNativeLoadingArtwork(true);
 }
}
