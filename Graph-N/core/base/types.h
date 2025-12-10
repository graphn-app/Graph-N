#pragma once

using byte = uint8_t;
using app_version_type = uint64_t;
using time_type = uint64_t;
using lid = uint32_t;	// local ID
using gid = uint64_t;	// global id
using byte_buff = std::vector<byte>;

enum class DlgObject
{
	none,

	GraphN_Flow,
	Options,

	_first				= GraphN_Flow,
	_last				= Options,
	_count = _last,
	_zero = none
};

enum DlgCtrl
{
	none = 0,

	btn_ok				= 0x00001,
	btn_cancel			= 0x00002,
	btn_yes				= 0x00004,
	btn_no				= 0x00008,

	ico_err				= 0x10000,
	ico_info			= 0x20000,
	ico_warn			= 0x40000,
	ico_ques			= 0x80000,

	_btn_count = 4,
	_ico_count = 4,
};

// update LogWriter with new elements!
enum class DlgCode
{
	progress,

	ok,
	cancel,
	yes,
	no,
};
