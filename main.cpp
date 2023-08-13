#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <filesystem>
#include <Windows.h>
#include <cpprest/http_client.h>

using namespace std;
using namespace web::http;
using namespace web::http::client;

int main(int argc, char* argv[]) {
    map<string, string> argumentMap;

    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            size_t equalsPos = arg.find('=');
            if (equalsPos != string::npos) {
                string key = arg.substr(0, equalsPos);
                string value = arg.substr(equalsPos + 1);
                argumentMap[key] = value;
            }
        }
    }

    string programName = argv[0];
    string fullPath = filesystem::weakly_canonical(programName).string();
    string directoryPath = filesystem::path(fullPath).parent_path().string();

    string appPath = filesystem::current_path().string();
    string fortniteExePath = appPath + "\\FortniteClient-Win64-Shipping.exe";
    string fortniteEOSPath = appPath + "\\FortniteClient-Win64-Shipping_EAC_EOS.exe";

    URLDownloadToFile(NULL, L"https://cdn.discordapp.com/attachments/1076329540403597362/1139729703691898880/FortniteClient-Win64-Shipping_EAC_EOS.exe", L"FortniteClient-Win64-Shipping_EAC_EOS.exe", 0, NULL);

    if (!filesystem::exists(fortniteExePath)) {
        cout << "FortniteClient-Win64-Shipping.exe not found in the current directory." << endl;
        return 0;
    }

    string authPassword = argumentMap["-AUTH_PASSWORD"];
    string epicusername = argumentMap["-epicusername"];
    string epicuserid = argumentMap["-epicuserid"];
    string obfuscationid = argumentMap["-obfuscationid"];

    wstring racp = L"https://caldera-service-prod.ecosec.on.epicgames.com/caldera/api/v1/launcher/racp";

    json::value data;
    data[L"account_id"] = json::value::string(utility::conversions::to_string_t(epicuserid));
    data[L"exchange_code"] = json::value::string(utility::conversions::to_string_t(authPassword));
    data[L"test_mode"] = json::value::boolean(false);
    data[L"epic_app"] = json::value::string(L"fortnite");
    data[L"nvidia"] = json::value::boolean(false);
    data[L"luna"] = json::value::boolean(false);
    data[L"salmon"] = json::value::boolean(false);

    http_client client(racp);

    http_request request(methods::POST);
    request.headers().set_content_type(L"application/json");
    request.set_body(data);

    auto response = client.request(request).get();
    if (response.status_code() != status_codes::OK) {
        cout << "HTTP request failed with status code: " << response.status_code() << endl;
        return 0;
    }

    json::value responseJson = response.extract_json().get();
    wstring jwt = responseJson[L"jwt"].as_string();

    wstring launchArguments = L"-obfuscationid=" + obfuscationid + " -AUTH_LOGIN=unused -AUTH_PASSWORD=" + authPassword + L" -AUTH_TYPE=exchangecode -epicapp=Fortnite -epicenv=Prod -EpicPortal -steamimportavailable -epicusername=" + epicusername + L" -epicuserid=" + epicuserid + L" -epiclocale=en -epicsandboxid=fn -nobe -noeac -fromfl=eaceos -caldera=" + jwt;

    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    CreateProcess(NULL, (LPWSTR)(fortniteEOSPath + L" " + launchArguments).c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    CreateProcess(NULL, (LPWSTR)(fortniteExePath + L" " + launchArguments).c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0);
    PROCESSENTRY32 pe = { sizeof(pe) };
    if (Process32First(hSnap, &pe)) {
        do {
            if (wstring(pe.szExeFile) == L"FortniteClient-Win64-Shipping_EAC_EOS.exe") {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, 0, pe.th32ProcessID);
                if (hProcess) {
                    TerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
                }
            }
        } while (Process32Next(hSnap, &pe));
    }
    CloseHandle(hSnap);

    Sleep(-1);
    return 0;
}
