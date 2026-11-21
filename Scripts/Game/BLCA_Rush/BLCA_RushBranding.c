class BLCA_RushBranding
{
 static const ResourceName LOGO_TEXTURE = "{D2A2D7C3746B5B19}Assets/BLCA/BLCA_Logo.edds";

 static bool ApplyLogo(ImageWidget logo, float size)
 {
  // Keep the native image in slot zero so shared loading widgets can be restored.
  if (!logo || !logo.LoadImageTexture(1, LOGO_TEXTURE) || !logo.SetImage(1))
   return false;

  int width, height;
  logo.GetImageSize(1, width, height);
  if (width > 0 && height > 0) logo.SetSize(size * width / height, size);
  logo.Update();
  return true;
 }
}
