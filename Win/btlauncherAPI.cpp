/**********************************************************\

  Auto-generated btlauncherAPI.cpp

\**********************************************************/

#include "JSObject.h"
#include "variant_list.h"
#include "DOM/Document.h"
#include "DOM/Window.h"
#include "global/config.h"
#include "btlauncherAPI.h"
#include "windows.h"
#include <map>
#include <string>
#include <stdio.h>
#include <string.h>
#include <atlbase.h>
#include <atlstr.h>
#include <string.h>

#define bufsz 2048
#define BT_HEXCODE "4823DF041B" // BT4823DF041B0D
#define BTLIVE_CODE "BTLive"
#define INSTALL_REG_PATH _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\")
#define MSIE_ELEVATION _T("Software\\Microsoft\\Internet Explorer\\Low Rights\\ElevationPolicy")
#define MSIE_ELEVATION_GUID "ECC81F59-D6B1-46A4-B5E8-900FB424B95D"

#ifdef LIVE
  #define PLUGIN_DL "http://s3.amazonaws.com/live-installer/LivePlugin.msi"
#else
  #define PLUGIN_DL "http://live.bittorrent.com/SoShare/SoShare.msi"
#endif

#define UT_DL "http://download.utorrent.com/latest/uTorrent.exe"
#define BT_DL "http://download.bittorrent.com/latest/BitTorrent.exe"
#define LV_DL "http://s3.amazonaws.com/live-installer/BTLive.exe"
#define TORQUE_DL "http://download.utorrent.com/torque/latest/Torque.exe"
#define SOSHARE_DL "http://download.utorrent.com/soshare/latest/SoShare.exe"

#define PAIRING_DOMAIN "getshareapp.com"
//#define PAIRING_DOMAIN "192.168.56.1"

#define LIVE_NAME "BTLive"
#define UTORRENT_NAME "uTorrent"
#define BITTORRENT_NAME "BitTorrent"
#define TORQUE_NAME "Torque"
#define SOSHARE_NAME "SoShare"

#define NOT_SUPPORTED_MESSAGE "This application is not supported."

BOOL write_elevation(const std::wstring& path, const std::wstring& name);

///////////////////////////////////////////////////////////////////////////////
/// @fn btlauncherAPI::btlauncherAPI(const btlauncherPtr& plugin, const FB::BrowserHostPtr host)
///
/// @brief  Constructor for your JSAPI object.  You should register your methods, properties, and events
///         that should be accessible to Javascript from here.
///
/// @see FB::JSAPIAuto::registerMethod
/// @see FB::JSAPIAuto::registerProperty
/// @see FB::JSAPIAuto::registerEvent
///////////////////////////////////////////////////////////////////////////////

btlauncherAPI::btlauncherAPI(const btlauncherPtr& plugin, const FB::BrowserHostPtr& host) : m_plugin(plugin), m_host(host)
{
	registerMethod("getInstallPath", make_method(this, &btlauncherAPI::getInstallPath));
	registerMethod("getInstallVersion", make_method(this, &btlauncherAPI::getInstallVersion));
	registerMethod("isRunning", make_method(this, &btlauncherAPI::isRunning));
	registerMethod("stopRunning", make_method(this, &btlauncherAPI::stopRunning));
	registerMethod("runProgram", make_method(this, &btlauncherAPI::runProgram));
	registerMethod("checkForUpdate", make_method(this, &btlauncherAPI::checkForUpdate));

	#ifdef SHARE
		registerMethod("downloadProgram", make_method(this, &btlauncherAPI::downloadProgram));
		registerMethod("enablePairing", make_method(this, &btlauncherAPI::enablePairing));
		registerMethod("ajax", make_method(this, &btlauncherAPI::ajax));
	#endif

    // Read-only property
    registerProperty("version",
                     make_property(this,
                        &btlauncherAPI::get_version));
}

///////////////////////////////////////////////////////////////////////////////
/// @fn btlauncherAPI::~btlauncherAPI()
///
/// @brief  Destructor.  Remember that this object will not be released until
///         the browser is done with it; this will almost definitely be after
///         the plugin is released.
///////////////////////////////////////////////////////////////////////////////
btlauncherAPI::~btlauncherAPI()
{
}

///////////////////////////////////////////////////////////////////////////////
/// @fn btlauncherPtr btlauncherAPI::getPlugin()
///
/// @brief  Gets a reference to the plugin that was passed in when the object
///         was created.  If the plugin has already been released then this
///         will throw a FB::script_error that will be translated into a
///         javascript exception in the page.
///////////////////////////////////////////////////////////////////////////////

btlauncherPtr btlauncherAPI::getPlugin()
{
    btlauncherPtr plugin(m_plugin.lock());
    if (!plugin) {
        throw FB::script_error("The plugin is invalid");
    }
    return plugin;
}



// Read/Write property testString
std::string btlauncherAPI::get_testString()
{
    return m_testString;
}
void btlauncherAPI::set_testString(const std::string& val)
{
    m_testString = val;
}

// Read-only property version
std::string btlauncherAPI::get_version()
{
    return FBSTRING_PLUGIN_VERSION;
}

void btlauncherAPI::do_callback(const FB::JSObjectPtr& callback, const std::vector<FB::variant>& args) {
	try {
		callback->InvokeAsync("", args);
	} catch (std::exception& e) {
		// TODO -- only catch the std::runtime_error("Cannot invoke asynchronously"); (FireBreath JSObject.h)
	}
}

void *load_dll_proc(std::string filename, std::string procname) {
	HMODULE mod = GetModuleHandleA(filename.c_str());
	if (mod==NULL) {
		mod = LoadLibraryA(filename.c_str());
		if (mod == NULL)
			return NULL;
	}
	return (void *) GetProcAddress(mod, procname.c_str());
}

BOOL RunAsAdministrator(wchar_t* name, wchar_t* cmdline, bool waitforsuccess,
						int successcode, bool hide)
{
	typedef BOOL WINAPI ShellExecuteExProc(LPSHELLEXECUTEINFO lpExecInfo);

	// Win95 doesn't have a stub for ShellExecuteExW so we need to dynload it
	ShellExecuteExProc* lpShellExecuteEx;

	lpShellExecuteEx =
		(ShellExecuteExProc*) load_dll_proc("shell32.dll", "ShellExecuteExW");

	assert(lpShellExecuteEx);

	BOOL result = FALSE;

	SHELLEXECUTEINFO sei;
	memset(&sei, 0, sizeof(SHELLEXECUTEINFO));

	sei.cbSize = sizeof(sei);
	sei.lpVerb = _T("runas");
	sei.lpFile = name;
	sei.lpParameters = cmdline;
	sei.nShow = hide ? SW_HIDE : SW_SHOWNORMAL;
	sei.fMask = SEE_MASK_NOASYNC | SEE_MASK_FLAG_NO_UI
				| (waitforsuccess ? SEE_MASK_NOCLOSEPROCESS : 0);

	if((*lpShellExecuteEx)(&sei) == TRUE)
	{
		if(!waitforsuccess)
		{
			result = TRUE;
		}
		else if(sei.hProcess != NULL)
			{
				// wait for the process to return exit code for success
			if(WaitForSingleObject(sei.hProcess, 60000) == WAIT_OBJECT_0)
			{
				DWORD dwExitCode;
				BOOL geresult = GetExitCodeProcess(sei.hProcess, &dwExitCode);
				if(geresult && dwExitCode == successcode)
				{
					result = TRUE;
				}
			}

			CloseHandle(sei.hProcess);
		}
	}

	return result;
}

BOOL RunningVistaOrGreater() {
	OSVERSIONINFO version;
		
	version.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
	::GetVersionEx(&version);

	return version.dwMajorVersion >= 6;
}

std::wstring GetRandomKey() {
	std::wstring possibilities = _T("0123456789ABCDEF");
	wchar_t tmp[40];
	int i;
	for(i = 0; i < 40; i++) {
		tmp[i] = possibilities.c_str()[rand() % possibilities.size()];
	}
	std::wstring ret(tmp, 40);
	assert(ret.size() == 40);
	return ret;
}

void btlauncherAPI::gotDownloadProgram(const FB::JSObjectPtr& callback, 
									   std::wstring& program,
									   bool success,
									   const FB::HeaderMap& headers,
									   const boost::shared_array<uint8_t>& data,
									   const size_t size) {
	TCHAR temppath[500];
	DWORD gettempresult = GetTempPath(500, temppath);
	if (! gettempresult) {
		do_callback(callback, FB::variant_list_of(false)("GetTempPath")(GetLastError()));
		return;
	}
	std::wstring folder(temppath);
	folder.append( program.c_str() );
	folder.append( _T("_") );
	boost::uuids::random_generator gen;
	boost::uuids::uuid u = gen();
	folder.append( boost::uuids::to_wstring(u) );
	BOOL result = CreateDirectory( folder.c_str(), NULL );
	if (! result) {
		do_callback(callback, FB::variant_list_of(false)("error creating directory")(GetLastError()));
		return;
	}
	std::wstring syspath(folder);
	std::wstring name(program);
	name.append(_T(".exe"));
	syspath.append( _T("\\") );
	syspath.append( name.c_str() );
	HANDLE hFile = CreateFile( syspath.c_str(), GENERIC_WRITE | GENERIC_EXECUTE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, NULL, NULL );
	if (hFile == INVALID_HANDLE_VALUE) {
		do_callback(callback, FB::variant_list_of(false)(GetLastError()));
		return;
	}
	PVOID ptr = (VOID*) data.get();
	DWORD written = 0;
	BOOL RESULT = WriteFile( hFile, ptr, size, &written, NULL);
	CloseHandle(hFile);
	
	if (! RESULT) {
		do_callback(callback, FB::variant_list_of("FILE")(false)(GetLastError()));
		return;
	}

	BOOL elevated = write_elevation(folder, name);


	std::wstring installcommand = std::wstring(syspath);
	std::wstring args;
	args.append(_T(" /PAIR "));
	std::wstring pairingkey = GetRandomKey();
	args.append(pairingkey);

	STARTUPINFO info;
	PROCESS_INFORMATION procinfo;
	memset(&info,0,sizeof(info));
	info.cb = sizeof(STARTUPINFO);
	 
	/* CreateProcessW can modify installcommand thus we allocate needed memory */ 
	wchar_t * pwszParam = new wchar_t[installcommand.size() + 1]; 
	wcscpy_s(pwszParam, installcommand.size() + 1, installcommand.c_str()); 

	wchar_t * pwszArgs = new wchar_t[args.size() + 1];
	wcscpy_s(pwszArgs, args.size() + 1, args.c_str());

	BOOL bProc = FALSE;
	if(RunningVistaOrGreater()) {
		BOOL blocking = false;
		bProc = RunAsAdministrator(pwszParam, pwszArgs, blocking, 0, false);
	} else {
		//Test on XP! pwszArgs may need to be Param+Args, where it is currently just args
		bProc = CreateProcess(pwszParam, pwszArgs, NULL, NULL, FALSE, 0, NULL, NULL, &info, &procinfo);
	}

	if(bProc) {
		do_callback( callback, FB::variant_list_of("PROCESS")(true)(installcommand.c_str())(GetLastError())(pairingkey));
	} else {
		do_callback( callback, FB::variant_list_of("PROCESS")(false)(installcommand.c_str())(GetLastError()));
	}

}

void btlauncherAPI::gotCheckForUpdate(const FB::JSObjectPtr& callback, 
									   bool success,
									   const FB::HeaderMap& headers,
									   const boost::shared_array<uint8_t>& data,
									   const size_t size) {
	if (! success) {
		do_callback(callback, FB::variant_list_of(success));
		return;
	}
	TCHAR temppath[500];
	DWORD gettempresult = GetTempPath(500, temppath);
	if (! gettempresult) {
		do_callback(callback, FB::variant_list_of(false)("GetTempPath")(GetLastError()));
		return;
	}
	std::wstring syspath(temppath);
	syspath.append( _T("btlaunch") );
	boost::uuids::random_generator gen;
	boost::uuids::uuid u = gen();
	syspath.append( _T("_") );
	syspath.append( boost::uuids::to_wstring(u) );
	syspath.append(_T(".msi"));

	HANDLE hFile = CreateFile( syspath.c_str(), GENERIC_WRITE | GENERIC_EXECUTE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, NULL, NULL );
	if (hFile == INVALID_HANDLE_VALUE) {
		do_callback(callback, FB::variant_list_of(false)("CreateFile")(GetLastError()));
		return;
	}
	PVOID ptr = (VOID*) data.get();
	DWORD written = 0;
	BOOL RESULT = WriteFile( hFile, ptr, size, &written, NULL);
	CloseHandle(hFile);
	
	if (! RESULT) {
		do_callback(callback, FB::variant_list_of("WriteFile")(false)(GetLastError()));
		return;
	}
	std::wstring installcommand = std::wstring(_T("msiexec.exe /I "));
	installcommand.append( std::wstring(syspath) );
	installcommand.append( _T(" /q") );
	//installcommand.append(_T(" /NOINSTALL /MINIMIZED /HIDE"));
	STARTUPINFO info;
	PROCESS_INFORMATION procinfo;
	memset(&info,0,sizeof(info));
	info.cb = sizeof(STARTUPINFO);
	 
	/* CreateProcessW can modify installcommand thus we allocate needed memory */ 
	wchar_t * pwszParam = new wchar_t[installcommand.size() + 1]; 
	const wchar_t* pchrTemp = installcommand.c_str(); 
    wcscpy_s(pwszParam, installcommand.size() + 1, pchrTemp); 

	BOOL bProc = CreateProcess(NULL, pwszParam, NULL, NULL, FALSE, 0, NULL, NULL, &info, &procinfo);
	if(bProc) {
		do_callback(callback, FB::variant_list_of("CreateProcess")(true)(installcommand.c_str())(GetLastError()));
	} else {
		do_callback(callback, FB::variant_list_of("CreateProcess")(false)(installcommand.c_str())(GetLastError()));
	}
}

void btlauncherAPI::checkForUpdate(const FB::JSObjectPtr& callback) {
	std::string url = std::string(PLUGIN_DL);
	url.append( std::string("?v=") );
	url.append( std::string(FBSTRING_PLUGIN_VERSION) );

	
	SYSTEMTIME lpTime;
	GetSystemTime(&lpTime);
	char str[1024];
	sprintf(str,"&_t=%d",(lpTime.wSecond + 1000 * lpTime.wMilliseconds));
	url.append(str);

	//url.append( itoa(lpTime->wMilliseconds, buf, 10) );
	FB::SimpleStreamHelper::AsyncGet(m_host, FB::URI::fromString(url), 
		boost::bind(&btlauncherAPI::gotCheckForUpdate, this, callback, _1, _2, _3, _4)
		);
}

void btlauncherAPI::ajax(const std::string& url, const FB::JSObjectPtr& callback) {
	if (FB::URI::fromString(url).domain != "127.0.0.1") {
		FB::VariantMap response;
		response["allowed"] = false;
		response["success"] = false;
		do_callback(callback, FB::variant_list_of(response));
		return;
	}
	FB::SimpleStreamHelper::AsyncGet(m_host, FB::URI::fromString(url), 
		boost::bind(&btlauncherAPI::gotajax, this, callback, _1, _2, _3, _4)
		);
}

void btlauncherAPI::gotajax(const FB::JSObjectPtr& callback, 
							bool success,
						    const FB::HeaderMap& headers,
						    const boost::shared_array<uint8_t>& data,
						    const size_t size) {
	FB::VariantMap outHeaders;
	for (FB::HeaderMap::const_iterator it = headers.begin(); it != headers.end(); ++it) {
        if (headers.count(it->first) > 1) {
            if (outHeaders.find(it->first) != outHeaders.end()) {
                outHeaders[it->first].cast<FB::VariantList>().push_back(it->second);
            } else {
                outHeaders[it->first] = FB::VariantList(FB::variant_list_of(it->second));
            }
        } else {
            outHeaders[it->first] = it->second;
        }
    }
	FB::VariantMap response;
	response["headers"] = outHeaders;
	response["allowed"] = true;
	response["success"] = success;
	response["size"] = size;
	std::string result = std::string((const char*) data.get(), size);
	response["data"] = result;
	
	do_callback(callback, FB::variant_list_of(response));
}

void btlauncherAPI::downloadProgram(const std::wstring& program, const FB::JSObjectPtr& callback) {
	std::string url;

	if (wcsstr(program.c_str(), _T("uTorrent"))) {
		url = std::string(UT_DL);
	} else if (wcsstr(program.c_str(), _T("BitTorrent"))) {
		url = std::string(BT_DL);
    } else if (wcsstr(program.c_str(), _T("Torque"))) {
		url = std::string(TORQUE_DL);
    } else if (wcsstr(program.c_str(), _T("SoShare"))) {
		url = std::string(SOSHARE_DL);
	} else if (wcsstr(program.c_str(), _T("BTLive"))) { 
		url = std::string(LV_DL);
	} else {
	  return;
	}
	
	//url = version.c_str();
		
	FB::SimpleStreamHelper::AsyncGet(m_host, FB::URI::fromString(url), 
		boost::bind(&btlauncherAPI::gotDownloadProgram, this, callback, program, _1, _2, _3, _4)
	);
}


std::wstring getRegStringValue(const std::wstring& path, const std::wstring& key, HKEY parentKey) {
	CRegKey regKey;
	const CString REG_SW_GROUP = path.c_str();
	TCHAR szBuffer[bufsz];
	ULONG cchBuffer = bufsz;
	LONG RESULT;
	RESULT = regKey.Open(parentKey, REG_SW_GROUP, KEY_READ);
	if (RESULT == ERROR_SUCCESS) {
		RESULT = regKey.QueryStringValue( key.c_str(), szBuffer, &cchBuffer );
		regKey.Close();
		if (RESULT == ERROR_SUCCESS) {
			return std::wstring(szBuffer);
		} else {
			return std::wstring(_T(""));
		}
	} else {
		return std::wstring(_T(""));
	}
}

BOOL setRegStringValue(const std::wstring& path, const std::wstring& key, const std::wstring& value, HKEY parentKey) {
	CRegKey regKey;
	const CString REG_SW_GROUP = path.c_str();
	LONG res;
	res = regKey.Create(parentKey, REG_SW_GROUP, REG_NONE);
	res = regKey.Open(parentKey, REG_SW_GROUP, KEY_READ | KEY_WRITE);
	DWORD err = GetLastError();
	if (res == ERROR_SUCCESS) {
		res = regKey.SetValue( value.c_str(), key.c_str() );
		regKey.Close();
		if (res == ERROR_SUCCESS) {
			return true;
		} else {
			return false;
		}
	} else {
		return false;
	}
}

BOOL setRegDwordValue(const std::wstring& path, const std::wstring& key, DWORD value, HKEY parentKey) {
	CRegKey regKey;
	const CString REG_SW_GROUP = path.c_str();
	LONG res;
	res = regKey.Create(parentKey, REG_SW_GROUP, REG_NONE);
	res = regKey.Open(parentKey, REG_SW_GROUP, KEY_READ | KEY_WRITE);
	DWORD err = GetLastError();
	if (res == ERROR_SUCCESS) {
		res = regKey.SetValue( value, key.c_str() );
		regKey.Close();
		if (res == ERROR_SUCCESS) {
			return true;
		} else {
			return false;
		}
	} else {
		return false;
	}
}

std::wstring btlauncherAPI::getInstallVersion(const std::wstring& program) {	
	if (!this->isSupported(program)) {
		return _T(NOT_SUPPORTED_MESSAGE);
	}
	std::wstring reg_group = std::wstring(INSTALL_REG_PATH).append( program );
	std::wstring ret = getRegStringValue( reg_group, _T("DisplayVersion"), HKEY_LOCAL_MACHINE );
	if (ret.empty()) {
		ret = getRegStringValue( reg_group, _T("DisplayVersion"), HKEY_CURRENT_USER );
	}
	return ret;
}

std::wstring get_install_path(const std::wstring& program) {
	std::wstring reg_group = std::wstring(INSTALL_REG_PATH).append( program );
	std::wstring ret = getRegStringValue( reg_group, _T("InstallLocation"), HKEY_LOCAL_MACHINE );
	if (ret.empty()) {
		ret = getRegStringValue( reg_group, _T("InstallLocation"), HKEY_CURRENT_USER );
	}
	return ret;
}

std::wstring btlauncherAPI::getInstallPath(const std::wstring& program) {
	if (!this->isSupported(program)) {
		return _T(NOT_SUPPORTED_MESSAGE);
	}
	return get_install_path(program);
}

std::wstring getExecutablePath(const std::wstring& program) {
	std::wstring reg_group = std::wstring(INSTALL_REG_PATH).append( program );
	std::wstring location = getRegStringValue( reg_group, _T("InstallLocation"), HKEY_LOCAL_MACHINE );
	if(location.empty()) {
		location = getRegStringValue( reg_group, _T("InstallLocation"), HKEY_CURRENT_USER ); 
	}
	location.append( _T("\\") );
	location.append( program );
	location.append( _T(".exe") );
	return location;
}

BOOL write_elevation(const std::wstring& path, const std::wstring& name) {
	std::wstring reg_group = std::wstring(MSIE_ELEVATION);
	reg_group.append(_T("\\{"));
	boost::uuids::random_generator gen;
	boost::uuids::uuid u = gen();
	//reg_group.append( boost::uuids::to_wstring(u) );
	reg_group.append(_T(MSIE_ELEVATION_GUID));
	reg_group.append(_T("}"));
	BOOL ret = setRegStringValue( reg_group, _T("AppPath"), path, HKEY_CURRENT_USER );
	ret = setRegStringValue( reg_group, _T("AppName"), name, HKEY_CURRENT_USER );
	ret = setRegDwordValue( reg_group, _T("Policy"), 3, HKEY_CURRENT_USER );
	return ret;
}

BOOL launch_program(const std::wstring& program, const std::wstring& switches) {
	//HINSTANCE result = ShellExecute(NULL, NULL, getExecutablePath(program).c_str(), NULL, NULL, NULL);
	// pops up a security dialog in IE

	// try to write to IE security dialog...
	BOOL result = write_elevation(get_install_path(program), _T("SoShare.exe"));

	std::wstring installcommand = getExecutablePath(program).c_str();
	if (switches.length() > 0) {
		installcommand.append( switches );
	}
	STARTUPINFO info;
	PROCESS_INFORMATION procinfo;
	memset(&info,0,sizeof(info));
	info.cb = sizeof(STARTUPINFO);

	wchar_t * pwszParam = new wchar_t[installcommand.size() + 1]; 
	const wchar_t* pchrTemp = installcommand.c_str(); 
    wcscpy_s(pwszParam, installcommand.size() + 1, pchrTemp); 
	BOOL bProc = FALSE;
	bProc = CreateProcess(NULL, pwszParam, NULL, NULL, FALSE, 0, NULL, NULL, &info, &procinfo);
	return bProc;
}

FB::variant btlauncherAPI::enablePairing(const std::wstring& program, const std::wstring& key) {
	if (!this->isSupported(program)) {
		return _T(NOT_SUPPORTED_MESSAGE);
	}
	std::string location = m_host->getDOMWindow()->getLocation();
	FB::URI uri = FB::URI::fromString(location);
	if (true || uri.domain.find(PAIRING_DOMAIN)!=std::string::npos) {
		
	} else {
		return _T("access denied");
	}
	//std::string location = w->getLocation();
	BOOL ret = FALSE;
	std::wstring switches = std::wstring(_T(" /PAIR "));
	switches.append(key);
	ret = launch_program(program, switches);
	return ret;
}

FB::variant btlauncherAPI::runProgram(const std::wstring& program, const FB::JSObjectPtr& callback) {
	if (!this->isSupported(program)) {
		return _T(NOT_SUPPORTED_MESSAGE);
	}
	
	//HINSTANCE ret = (HINSTANCE)0;
	BOOL ret = FALSE;
	if (isRunning(program).size() == 0) {
		ret = launch_program(program, _T(""));
		do_callback(callback, FB::variant_list_of(false)(ret));
	}
	return ret;
}

struct callbackdata {
	callbackdata() {
		found = FALSE;
	}
	BOOL found;
	std::wstring name;
	FB::VariantList list;
};

TCHAR classname[50];
BOOL CALLBACK EnumWindowCB(HWND hWnd, LPARAM lParam) {
	GetClassName(hWnd, classname, sizeof(classname));
	callbackdata* cbdata = (callbackdata*) lParam;
	// BT4823 see gui/wndmain.cpp for the _utorrent_classname (begins with 4823)
	if (wcsstr(classname, cbdata->name.c_str()) && 
		(wcsstr(classname, _T(BT_HEXCODE)) || wcsstr(classname, _T(BTLIVE_CODE)))) {	
		//FB::JSObjectPtr& callback = *((FB::JSObjectPtr*)lParam);
		//cbdata->found = true;
		cbdata->list.push_back( std::wstring(classname) );
		//return FALSE;
	}
	return TRUE;
}

FB::VariantList btlauncherAPI::stopRunning(const std::wstring& val) {
	FB::VariantList list;
	if (wcsstr(val.c_str(), _T(BT_HEXCODE)) || wcsstr(val.c_str(), _T(BTLIVE_CODE))) {
		HWND hWnd = FindWindow( val.c_str(), NULL );
		DWORD pid;
		DWORD parent;
		parent = GetWindowThreadProcessId(hWnd, &pid);
		HANDLE pHandle = OpenProcess(PROCESS_TERMINATE, NULL, pid);
		if (! pHandle) {
			list.push_back("could not open process");
			list.push_back(GetLastError());
		} else {
			BOOL result = TerminateProcess(pHandle, 0);
			list.push_back("ok");
			list.push_back(result);
		}
	}
	return list;
}

FB::VariantList btlauncherAPI::isRunning(const std::wstring& val) {
	FB::VariantList list;
	callbackdata cbdata;
	cbdata.list = list;
	cbdata.name = val;
	EnumWindows(EnumWindowCB, (LPARAM) &cbdata);
	return cbdata.list;	
}

bool btlauncherAPI::isSupported(std::wstring program) {

	if (program == _T(LIVE_NAME) || program == _T(UTORRENT_NAME) || program == _T(BITTORRENT_NAME) || program == _T(TORQUE_NAME) || program == _T(SOSHARE_NAME)) {
		return true;
	}
	return false;
}


//FB::variant btlauncherAPI::launchClient(const std::
