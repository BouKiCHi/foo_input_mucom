#include "stdafx.h"
#include "resource.h"
#include <helpers/atl-misc.h>
#include <helpers/dropdown_helper.h>

// {89C8D303-BE7C-43D5-B948-184968DB8B6E}
static const GUID guid_cfg_rate = { 0x89c8d303, 0xbe7c, 0x43d5, { 0xb9, 0x48, 0x18, 0x49, 0x68, 0xdb, 0x8b, 0x6e } };
// {11B6D599-F102-438D-8DF3-71A9D417564C}
static const GUID guid_cfg_deflen = { 0x11b6d599, 0xf102, 0x438d, { 0x8d, 0xf3, 0x71, 0xa9, 0xd4, 0x17, 0x56, 0x4c } };

enum {
	default_cfg_rate = 44100,
	default_cfg_deflen = 180,
};

cfg_uint cfg_rate(guid_cfg_rate, default_cfg_rate);
cfg_uint cfg_deflen(guid_cfg_deflen, default_cfg_deflen);

static const unsigned srate_tab[] = { 8000, 11025, 16000, 22050, 24000, 32000, 44100, 48000 };

class CMyPreferences : public CDialogImpl<CMyPreferences>, public preferences_page_instance {
public:
	CMyPreferences(preferences_page_callback::ptr callback) : m_callback(callback) {}

	//dialog resource ID
	enum {IDD = IDD_DIAG1};
	t_uint32 get_state();
	void apply();
	void reset();

	//WTL message map
	BEGIN_MSG_MAP_EX(CMyPreferences)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDC_RATE, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_DEFLEN, EN_CHANGE, OnEditChange)
	END_MSG_MAP()
private:
	BOOL OnInitDialog(CWindow, LPARAM);
	void OnEditChange(UINT, int, CWindow);
	bool HasChanged();
	void OnChanged();

	const preferences_page_callback::ptr m_callback;
};

BOOL CMyPreferences::OnInitDialog(CWindow, LPARAM) {
	char temp[16] = {};

	SetDlgItemInt(IDC_RATE, cfg_rate, FALSE);
	SetDlgItemInt(IDC_DEFLEN, cfg_deflen, FALSE);

	return FALSE;
}

void CMyPreferences::OnEditChange(UINT, int, CWindow) {
	OnChanged();
}


t_uint32 CMyPreferences::get_state() {
	t_uint32 state = preferences_state::resettable;
	if (HasChanged()) state |= preferences_state::changed;
	return state;
}

void CMyPreferences::reset() {
	SetDlgItemInt(IDC_RATE, cfg_rate, FALSE);
	SetDlgItemInt(IDC_DEFLEN, cfg_deflen, FALSE);
	OnChanged();
}

void CMyPreferences::apply() {
	unsigned fs = GetDlgItemInt(IDC_RATE, NULL, FALSE);
	if (fs < 1000) fs = 1000;
	SetDlgItemInt(IDC_RATE, fs, FALSE);
	cfg_rate = fs;

	unsigned deflen = GetDlgItemInt(IDC_DEFLEN, NULL, FALSE);
	SetDlgItemInt(IDC_DEFLEN, deflen, FALSE);
	cfg_deflen = deflen;

	OnChanged(); 
}

bool CMyPreferences::HasChanged() {
	if (GetDlgItemInt(IDC_RATE, NULL, FALSE) != cfg_rate) return true;
	if (GetDlgItemInt(IDC_DEFLEN, NULL, FALSE) != cfg_deflen) return true;
	return false;
}
void CMyPreferences::OnChanged() {
	m_callback->on_state_changed();
}

class preferences_page_myimpl : public preferences_page_impl<CMyPreferences> {
public:
	const char * get_name() {return "MUCOM88";}
	GUID get_guid() {
		// {BE7E260E-93BF-4016-88A4-ED449D9DB487}
		static const GUID guid = { 0xbe7e260e, 0x93bf, 0x4016, { 0x88, 0xa4, 0xed, 0x44, 0x9d, 0x9d, 0xb4, 0x87 } };
		return guid;
	}
	GUID get_parent_guid() {return guid_input;}
};

static preferences_page_factory_t<preferences_page_myimpl> g_preferences_page_myimpl_factory;
