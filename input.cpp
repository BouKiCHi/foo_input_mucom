#include "stdafx.h"
#include <locale.h>

#include "mucom88/src/module/mucom_module.h"
#include "mucom88/src/utils/pathutil.h"

extern cfg_uint cfg_rate, cfg_deflen;

class input_song : public input_stubs {
	unsigned rate;
	unsigned channels;
	unsigned bitDepth;
	unsigned bytesPerFrame;

public:
	PathUtil pathu;
	const char *path;
	MucomModule module;

	input_song() {
		rate = cfg_rate;
		channels = 2;
		bitDepth = 16;
		// １フレームのバイト数
		bytesPerFrame = bitDepth / 8 * channels;
		path = NULL;
	}


	void open(service_ptr_t<file> p_filehint,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort) {

		// タグ書き込みはできない
		if (p_reason == input_open_info_write) throw exception_io_unsupported_format();

		path = p_path;
		pathu.Split(p_path);
		m_file = p_filehint;

		input_open_file_helper(m_file, p_path, p_reason, p_abort);
		load_data(p_abort);
	}

	void load_data(abort_callback& p_abort) {
		// ファイルサイズ
		t_size size = (t_size)m_file->get_size(p_abort);

		// 長さ不明
		if (size == filesize_invalid) {
			throw exception_io_unsupported_format();
		}

		// 曲データ読み出し
		m_data.set_size(size + 1);
		m_file->read(m_data.get_ptr(), size, p_abort);
		m_data[size] = 0;

		// ロケール設定(UTF8パス処理のため)
		const char* p = setlocale(LC_ALL, NULL);
		std::string oldlocale(p);
		setlocale(LC_ALL, ".UTF8");

		const char *dir = pathu.GetDirectory();
		module.SetWorkDir(dir);

		module.Close();

		// 設定
		module.SetDefaultLength(cfg_deflen);
		module.SetRate(cfg_rate);

		module.OpenMemory(m_data.get_ptr(), size, path);
		module.UseFader(true);
		module.Play();
		setlocale(LC_ALL, oldlocale.c_str());
	}

	void get_info(file_info & p_info,abort_callback & p_abort) {
		
		// 長さ設定
		p_info.set_length(module.GetLength());

		// 曲名
		std::string title = SJIStoUTF8(module.tag->title);
		std::string composer = SJIStoUTF8(module.tag->composer);
		std::string author = SJIStoUTF8(module.tag->author);
		p_info.meta_add("TITLE", title.c_str());
		p_info.meta_add("ALBUM", title.c_str());
		p_info.meta_add("ARTIST", composer.c_str());
		p_info.meta_add("PERFORMER", author.c_str());
	}

	t_filestats get_file_stats(abort_callback & p_abort) {return m_file->get_stats(p_abort);}

	void decode_initialize(unsigned p_flags,abort_callback & p_abort) {
		m_file->reopen(p_abort); 
	}

	bool decode_run(audio_chunk & p_chunk,abort_callback & p_abort) {
		int frames = 1024;

		// 1024フレーム分を生成する
		const size_t bytes = frames * bytesPerFrame;
		m_buffer.set_size(bytes);
		module.Mix((short*)m_buffer.get_ptr(), frames);

		// EOF?
		if (module.IsEnd()) return false;

		// 変換
		if (0 < bitDepth) {
			p_chunk.set_data_fixedpoint(m_buffer.get_ptr(), frames * bytesPerFrame, rate, channels, bitDepth, audio_chunk::g_guess_channel_config(channels));
		}
		else {
			p_chunk.set_data((const audio_sample*)m_buffer.get_ptr(), frames, channels, rate);
		}

		return true;
	}

	void decode_seek(double p_seconds,abort_callback & p_abort) {
		m_file->ensure_seekable();
	}

	bool decode_can_seek() { return false; }

	bool decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta) { 
		// 曲によって動的に変更される

		// 長さは設定によって違うので、ここに。
		p_out.set_length(module.GetLength());

		// 現在の状態
		p_out.info_set_int("samplerate", rate);
		p_out.info_set_int("channels", channels);
		return true;
		//return false;
	}

	bool decode_get_dynamic_info_track(file_info & p_out, double & p_timestamp_delta) {
		return false;
	}

	void decode_on_idle(abort_callback & p_abort) {
		m_file->on_idle(p_abort);
	}

	void retag(const file_info & p_info,abort_callback & p_abort) {
		throw exception_io_unsupported_format();
	}

	static bool g_is_our_content_type(const char * p_content_type) {
		return false;
	} 

	static bool g_is_our_path(const char * p_path, const char * p_extension) {
		return 0 == stricmp_utf8(p_extension, "mub") || 0 == stricmp_utf8(p_extension, "muc");
	}

	static const char * g_get_name() { return "foo_input_mucom input"; }
	static const GUID g_get_guid() {
		// {837D22F1-0A06-45A1-AD5D-A05CB913EDD5}
		return { 0x837d22f1, 0xa06, 0x45a1, { 0xad, 0x5d, 0xa0, 0x5c, 0xb9, 0x13, 0xed, 0xd5 } };
	}

private:
	service_ptr_t<file> m_file;
	pfc::array_t<t_uint8> m_buffer;
	pfc::array_t<t_uint8> m_data;

	std::string SJIStoUTF8(std::string sjisstr)
	{
		// SJISからUnicodeへ変換
		const char* sjisbytes = sjisstr.c_str();
		int unilen = MultiByteToWideChar(CP_THREAD_ACP, 0, sjisbytes, -1, NULL, 0);
		wchar_t* unibytes = new wchar_t[unilen];
		MultiByteToWideChar(CP_THREAD_ACP, 0, sjisbytes, -1, unibytes, unilen);

		// UnicodeからUTF8へ変換
		int utf8len = WideCharToMultiByte(CP_UTF8, 0, unibytes, -1, NULL, 0, NULL, NULL);
		char* utf8bytes = new char[utf8len];
		WideCharToMultiByte(CP_UTF8, 0, unibytes, -1, utf8bytes, utf8len, NULL, NULL);

		std::string utf8str(utf8bytes);

		delete[] unibytes;
		delete[] utf8bytes;

		return utf8str;
	}
};

static input_singletrack_factory_t<input_song> g_input_song_factory;

// ファイル種別
DECLARE_FILE_TYPE("MUCOM88 files","*.MUB;*.MUC");

DECLARE_COMPONENT_VERSION("MUCOM88 for foobar2000", "1.0",
		"mucom88 .mub .muc player component \n");

VALIDATE_COMPONENT_FILENAME("foo_input_mucom.dll");
