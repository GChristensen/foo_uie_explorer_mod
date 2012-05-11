#include "stdafx.h"
#include "cmdmap.h"
#include "Preference.h"
#include "resource.h"

namespace prefs
{
	// {7DAFA74B-1B6E-44e0-ADFF-B7CA65A128C0}
	static const GUID guidStartup = 
	{ 0x7dafa74b, 0x1b6e, 0x44e0, { 0xad, 0xff, 0xb7, 0xca, 0x65, 0xa1, 0x28, 0xc0 } };
	cfg_int_resetable iStartup(guidStartup, IDC_ST_LAST);
	// {616E5B86-BAE7-4187-81B0-37831079F96C}
	static const GUID guidStartupPath = 
	{ 0x616e5b86, 0xbae7, 0x4187, { 0x81, 0xb0, 0x37, 0x83, 0x10, 0x79, 0xf9, 0x6c } };
	cfg_string_resetable sStartupPath(guidStartupPath, "");
	// {73B7A43D-39B5-4d47-B0FB-5F2154F2475F}
	static const GUID guidLastPath = 
	{ 0x73b7a43d, 0x39b5, 0x4d47, { 0xb0, 0xfb, 0x5f, 0x21, 0x54, 0xf2, 0x47, 0x5f } };
	cfg_string_resetable sLastPath(guidLastPath, "");
	// {93D4EF21-E950-4837-9F29-8FD2465AE708}
	static const GUID guidShowAddBar = 
	{ 0x93d4ef21, 0xe950, 0x4837, { 0x9f, 0x29, 0x8f, 0xd2, 0x46, 0x5a, 0xe7, 0x8 } };
	cfg_bool_resetable bShowAddBar(guidShowAddBar, 1);
	// {A65E90A0-E848-4229-A0FE-FC2727E40D63}
	static const GUID guidShowLines = 
	{ 0xa65e90a0, 0xe848, 0x4229, { 0xa0, 0xfe, 0xfc, 0x27, 0x27, 0xe4, 0xd, 0x63 } };
	cfg_bool_resetable bShowLines(guidShowLines, 1);
	// {9EBD1ECF-E33F-4abe-A41E-38C69482A243}
	static const GUID guidShowIcons = 
	{ 0x9ebd1ecf, 0xe33f, 0x4abe, { 0xa4, 0x1e, 0x38, 0xc6, 0x94, 0x82, 0xa2, 0x43 } };
	cfg_bool_resetable bShowIcons(guidShowIcons, 1);
	// {F1B5211E-DBEF-4503-A7BA-A925CA112440}
	static const GUID guidShowHScroll = 
	{ 0xf1b5211e, 0xdbef, 0x4503, { 0xa7, 0xba, 0xa9, 0x25, 0xca, 0x11, 0x24, 0x40 } };
	cfg_bool_resetable bShowHScroll(guidShowHScroll, 1);
	// {F8D42665-9C76-49fb-9970-087F0E378CDE}
	static const GUID guidShowToolTip = 
	{ 0xf8d42665, 0x9c76, 0x49fb, { 0x99, 0x70, 0x8, 0x7f, 0xe, 0x37, 0x8c, 0xde } };
	cfg_bool_resetable bShowTooltip(guidShowToolTip, 1);
	// {7DC49BB1-CC5A-48de-AC3E-AA90158F331A}
	static const GUID guidForceSort = 
	{ 0x7dc49bb1, 0xcc5a, 0x48de, { 0xac, 0x3e, 0xaa, 0x90, 0x15, 0x8f, 0x33, 0x1a } };
	cfg_bool_resetable bForceSort(guidForceSort, 0);
	// {1D568591-087B-47dd-92CE-D16642E7C6AE}
	static const GUID guidShowTypeMode = 
	{ 0x1d568591, 0x87b, 0x47dd, { 0x92, 0xce, 0xd1, 0x66, 0x42, 0xe7, 0xc6, 0xae } };
	cfg_int_resetable iShowTypeMode(guidShowTypeMode, IDC_FILES_ALL);
	// {1C77E0A3-7F36-49b6-8768-F33DE2EDFD62}
	static const GUID guidShowTypeArray = 
	{ 0x1c77e0a3, 0x7f36, 0x49b6, { 0x87, 0x68, 0xf3, 0x3d, 0xe2, 0xed, 0xfd, 0x62 } };
	cfg_string_resetable sShowType(guidShowTypeArray, "");
	// {D9A150CD-63ED-4bf9-BD18-1550668D9FC3}
	static const GUID guidShowHiddenFiles = 
	{ 0xd9a150cd, 0x63ed, 0x4bf9, { 0xbd, 0x18, 0x15, 0x50, 0x66, 0x8d, 0x9f, 0xc3 } };
	cfg_bool_resetable bShowHiddenFiles(guidShowHiddenFiles, 0);
	// {A34258B5-BFAF-4bd9-9845-83084D73A405}
	static const GUID guidShowFileExtension = 
	{ 0xa34258b5, 0xbfaf, 0x4bd9, { 0x98, 0x45, 0x83, 0x8, 0x4d, 0x73, 0xa4, 0x5 } };
	cfg_bool_resetable bShowFileExtension(guidShowFileExtension, 1);
	static const GUID guidTreeFrame = 
	{ 0xf046b89b, 0xaa59, 0x4afa, { 0xb4, 0x67, 0x27, 0x54, 0xaa, 0x97, 0x32, 0x42 } };
	cfg_int_resetable iTreeFrame(guidTreeFrame, ID_TREE_GREY);
	// {B7A753D3-01DD-4c7a-B20C-EE0EDE53281F}
	static const GUID guidAdBarFrame = 
	{ 0xb7a753d3, 0x1dd, 0x4c7a, { 0xb2, 0xc, 0xee, 0xe, 0xde, 0x53, 0x28, 0x1f } };
	cfg_int_resetable iAdBarFrame(guidAdBarFrame, ID_ADBAR_GREY);
	// {0C5B016E-B8D9-452b-9992-ECA750ED9729}
	static const GUID guidHideDrive = 
	{ 0xc5b016e, 0xb8d9, 0x452b, { 0x99, 0x92, 0xec, 0xa7, 0x50, 0xed, 0x97, 0x29 } };
	cfg_string_resetable sHideDrive(guidHideDrive, "");
	// {237EF907-FB49-40c5-8C7D-309604DBC48A}
	static const GUID guidNodeHeight = 
	{ 0x237ef907, 0xfb49, 0x40c5, { 0x8c, 0x7d, 0x30, 0x96, 0x4, 0xdb, 0xc4, 0x8a } };
	cfg_int_resetable iNodeHeight(guidNodeHeight, 0);
	// {91C25101-F992-4b48-873F-99E54CE3D77F}
	static const GUID guidMinHeight = 
	{ 0x91c25101, 0xf992, 0x4b48, { 0x87, 0x3f, 0x99, 0xe5, 0x4c, 0xe3, 0xd7, 0x7f } };
	cfg_int_resetable iMinHeight(guidMinHeight, 0);
	// {A64EF7E2-7757-467f-9347-0D0806F0D213}
	static const GUID guidShortcuts = 
	{ 0xa64ef7e2, 0x7757, 0x467f, { 0x93, 0x47, 0xd, 0x8, 0x6, 0xf0, 0xd2, 0x13 } };
	cfg_array<_sKeyData> aShortcuts(guidShortcuts);
	// {FD1B06BC-5D9E-48ea-9C63-F7FCE1E1696C}
	static const GUID guidCmdList = 
	{ 0xfd1b06bc, 0x5d9e, 0x48ea, { 0x9c, 0x63, 0xf7, 0xfc, 0xe1, 0xe1, 0x69, 0x6c } };
	cfg_string_resetable sCmdList(guidCmdList, "");
	// {32050438-019D-41c8-90A7-96072930D456}
	static const GUID guidGrpCmd = 
	{ 0x32050438, 0x19d, 0x41c8, { 0x90, 0xa7, 0x96, 0x7, 0x29, 0x30, 0xd4, 0x56 } };
	cfg_bool_resetable bGrpCmd(guidGrpCmd, 0);
	// {7F3069C7-FAC3-4314-BA7F-68BA66DA62FF}
	static const GUID guidFavoList = 
	{ 0x7f3069c7, 0xfac3, 0x4314, { 0xba, 0x7f, 0x68, 0xba, 0x66, 0xda, 0x62, 0xff } };
	cfg_string_resetable sFavoList(guidFavoList, "");
	// {D1958189-AF1E-43fb-B5FE-D3493FC8ABC6}
	static const GUID guidFavoRoot = 
	{ 0xd1958189, 0xaf1e, 0x43fb, { 0xb5, 0xfe, 0xd3, 0x49, 0x3f, 0xc8, 0xab, 0xc6 } };
	cfg_int_resetable iShowFavoAsRoot(guidFavoRoot, 2);
	// {7C443988-22B6-40c5-B908-B851E8540DF2}
	static const GUID guidGrpFavo = 
	{ 0x7c443988, 0x22b6, 0x40c5, { 0xb9, 0x8, 0xb8, 0x51, 0xe8, 0x54, 0xd, 0xf2 } };
	cfg_bool_resetable bGrpFavo(guidGrpFavo, 1);
	// {681CF304-A091-4432-A214-2A506DEA7D08}
	static const GUID guidFilterType = 
	{ 0x681cf304, 0xa091, 0x4432, { 0xa2, 0x14, 0x2a, 0x50, 0x6d, 0xea, 0x7d, 0x8 } };
	cfg_string_resetable sFilterType(guidFilterType, "");
	// {E9F9E2C7-2739-4780-9B30-F042BB5A4616}
	static const GUID guidRecursive = 
	{ 0xe9f9e2c7, 0x2739, 0x4780, { 0x9b, 0x30, 0xf0, 0x42, 0xbb, 0x5a, 0x46, 0x16 } };
	cfg_bool_resetable bRecursive(guidRecursive, 0);
	// {87D0B190-827F-4fdc-93B2-71E8B992324B}
	static const GUID guidExpand = 
	{ 0x87d0b190, 0x827f, 0x4fdc, { 0x93, 0xb2, 0x71, 0xe8, 0xb9, 0x92, 0x32, 0x4b } };
	cfg_bool_resetable bExpand(guidExpand, 0);
	// {C317676A-AFEE-4027-9CE1-C4A148370CEF}
	static const GUID guidSendDef = 
	{ 0xc317676a, 0xafee, 0x4027, { 0x9c, 0xe1, 0xc4, 0xa1, 0x48, 0x37, 0xc, 0xef } };
	cfg_bool_resetable bSendDef(guidSendDef, 0);
	// {4885938A-CD74-439c-A8CB-40FD5F598B35}
	static const GUID guidDefPL = 
	{ 0x4885938a, 0xcd74, 0x439c, { 0xa8, 0xcb, 0x40, 0xfd, 0x5f, 0x59, 0x8b, 0x35 } };
	cfg_string_resetable sDefPL(guidDefPL, "Default");

}

int string_color(const char *clr)
{
	if (strlen(clr) != 6)
		return -1;

	const char *start = clr;

	for(;*start;start++)
		if (!isxdigit(*start))
			return -1;

	int r, g, b;

	if (sscanf(clr, "%2X%2X%2X", &r, &g, &b) != 3)
		return -1;

	return RGB(r, g, b);
}
