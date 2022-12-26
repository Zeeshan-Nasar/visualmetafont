#include "luadigitalkhatt.h"

static const struct luahb_constant_t ot_constants[] = {
  { "NAME_ID_COPYRIGHT",             HB_OT_NAME_ID_COPYRIGHT },
  { "NAME_ID_FONT_FAMILY",           HB_OT_NAME_ID_FONT_FAMILY },
  { "NAME_ID_FONT_SUBFAMILY",        HB_OT_NAME_ID_FONT_SUBFAMILY },
  { "NAME_ID_UNIQUE_ID",             HB_OT_NAME_ID_UNIQUE_ID },
  { "NAME_ID_FULL_NAME",             HB_OT_NAME_ID_FULL_NAME },
  { "NAME_ID_VERSION_STRING",        HB_OT_NAME_ID_VERSION_STRING },
  { "NAME_ID_POSTSCRIPT_NAME",       HB_OT_NAME_ID_POSTSCRIPT_NAME },
  { "NAME_ID_TRADEMARK",             HB_OT_NAME_ID_TRADEMARK },
  { "NAME_ID_MANUFACTURER",          HB_OT_NAME_ID_MANUFACTURER },
  { "NAME_ID_DESIGNER",              HB_OT_NAME_ID_DESIGNER },
  { "NAME_ID_DESCRIPTION",           HB_OT_NAME_ID_DESCRIPTION },
  { "NAME_ID_VENDOR_URL",            HB_OT_NAME_ID_VENDOR_URL },
  { "NAME_ID_DESIGNER_URL",          HB_OT_NAME_ID_DESIGNER_URL },
  { "NAME_ID_LICENSE",               HB_OT_NAME_ID_LICENSE },
  { "NAME_ID_LICENSE_URL",           HB_OT_NAME_ID_LICENSE_URL },
  { "NAME_ID_TYPOGRAPHIC_FAMILY",    HB_OT_NAME_ID_TYPOGRAPHIC_FAMILY },
  { "NAME_ID_TYPOGRAPHIC_SUBFAMILY", HB_OT_NAME_ID_TYPOGRAPHIC_SUBFAMILY },
  { "NAME_ID_MAC_FULL_NAME",         HB_OT_NAME_ID_MAC_FULL_NAME },
  { "NAME_ID_SAMPLE_TEXT",           HB_OT_NAME_ID_SAMPLE_TEXT },
  { "NAME_ID_CID_FINDFONT_NAME",     HB_OT_NAME_ID_CID_FINDFONT_NAME },
  { "NAME_ID_WWS_FAMILY",            HB_OT_NAME_ID_WWS_FAMILY },
  { "NAME_ID_WWS_SUBFAMILY",         HB_OT_NAME_ID_WWS_SUBFAMILY },
  { "NAME_ID_LIGHT_BACKGROUND",      HB_OT_NAME_ID_LIGHT_BACKGROUND },
  { "NAME_ID_DARK_BACKGROUND",       HB_OT_NAME_ID_DARK_BACKGROUND },
  { "NAME_ID_VARIATIONS_PS_PREFIX",  HB_OT_NAME_ID_VARIATIONS_PS_PREFIX },
  { "NAME_ID_INVALID",               HB_OT_NAME_ID_INVALID },
  { "LAYOUT_NO_SCRIPT_INDEX",        HB_OT_LAYOUT_NO_SCRIPT_INDEX },
  { "LAYOUT_NO_FEATURE_INDEX",       HB_OT_LAYOUT_NO_FEATURE_INDEX },
  { "LAYOUT_DEFAULT_LANGUAGE_INDEX", HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX },
  { "LAYOUT_NO_VARIATIONS_INDEX",    HB_OT_LAYOUT_NO_VARIATIONS_INDEX },
  { NULL, 0 }
};

static const struct luaL_Reg ot_methods[] = {
  { NULL, NULL }
};

static const struct luaL_Reg ot_functions[] = {
  { NULL,  NULL }
};

int dk_register_ot(lua_State *L) {
  return dk_register_class(L, "dkharfbuzz.ot", ot_methods, ot_functions, ot_constants);
}
