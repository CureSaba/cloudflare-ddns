#include <iostream>
#include <fstream>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <vector>
#include <array>
#include <string>

static std::string filename = "ddns.json";

std::string get_ip(const std::string &provider = "https://api.fixitlater.org/ip/") {
    cpr::Response res = cpr::Get(cpr::Url{provider});
    if (res.status_code == 200) {
        std::string ip = res.text;
        return ip;
    } else {
        std::cerr << "Failed to get ip: " << res.status_code << std::endl;
        return "";
    }
}

std::vector<std::array<std::string, 2> > list_zones(const std::string &api_token) {
    cpr::Response res = cpr::Get(cpr::Url{"https://api.cloudflare.com/client/v4/zones"},
                                 cpr::Header{
                                     {"Authorization", "Bearer " + api_token},
                                     {"Content-Type", "application/json"}
                                 });
    std::vector<std::array<std::string, 2> > zones;
    if (res.status_code == 200) {
        nlohmann::json j = nlohmann::json::parse(res.text);
        for (const auto &result: j["result"]) {
            zones.push_back(std::array<std::string, 2>{
                result["name"].get<std::string>(),
                result["id"].get<std::string>()
            });
        }
    } else {
        std::cerr << "Failed to list zones: " << res.status_code << std::endl;
    }
    return zones;
}

std::vector<std::array<std::string, 2> > find_A_record(const std::string &api_token, const std::string &zone_id) {
    cpr::Response res = cpr::Get(cpr::Url{"https://api.cloudflare.com/client/v4/zones/" + zone_id + "/dns_records"},
                                 cpr::Header{
                                     {"Authorization", "Bearer " + api_token},
                                     {"Content-Type", "application/json"}
                                 });
    std::vector<std::array<std::string, 2> > records;
    if (res.status_code == 200) {
        nlohmann::json j = nlohmann::json::parse(res.text);
        for (const auto &result: j["result"]) {
            if (result["type"] == "A") {
                records.push_back(std::array<std::string, 2>{
                    result["name"].get<std::string>(),
                    result["id"].get<std::string>()
                });
            }
        }
    } else {
        std::cerr << "Failed to list dns records: " << res.status_code << std::endl;
    }
    return records;
}

bool create_record(const std::string &api_token, const std::string &zone_id, const std::string &domain,
                   const std::string &ip) {
    cpr::Response res = cpr::Post(cpr::Url{"https://api.cloudflare.com/client/v4/zones/" + zone_id + "/dns_records"},
                                  cpr::Header{
                                      {"Authorization", "Bearer " + api_token},
                                      {"Content-Type", "application/json"}
                                  },
                                  cpr::Body{
                                      R"({"type":"A","name":")" + domain + R"(","content":")" + ip +
                                      R"(","ttl":120,"proxied":false})"
                                  });
    if (res.status_code == 200) {
        nlohmann::json j = nlohmann::json::parse(res.text);
        if (j["success"].get<bool>()) {
            return true;
        } else {
            std::cerr << "Failed to create record for " << domain << ": " << j["errors"] << std::endl;
            return false;
        }
    } else {
        std::cerr << "Failed to create record for " << domain << ": " << res.status_code << std::endl;
        return false;
    }
}

bool update_record(const std::string &api_token, const std::string &zone_id, const std::string &record_id,
                   const std::string &domain, const std::string &ip) {
    cpr::Response res = cpr::Patch(cpr::Url{
                                       "https://api.cloudflare.com/client/v4/zones/" + zone_id + "/dns_records/" +
                                       record_id
                                   },
                                   cpr::Header{
                                       {"Authorization", "Bearer " + api_token},
                                       {"Content-Type", "application/json"}
                                   },
                                   cpr::Body{
                                       R"({"content":")" + ip + R"("})"
                                   });
    if (res.status_code == 200) {
        nlohmann::json j = nlohmann::json::parse(res.text);
        if (j["success"].get<bool>()) {
            return true;
        } else {
            std::cerr << "Failed to update record for " << domain << ": " << j["errors"] << std::endl;
            return false;
        }
    } else {
        std::cerr << "Failed to update record for " << domain << ": " << res.status_code << std::endl;
        return false;
    }
}

bool is_domain_in_zone(const std::string& domain, const std::string& zone_name) {
    if (domain == zone_name) return true;
    std::string suffix = "." + zone_name;
    if (domain.length() > suffix.length()) {
        return domain.compare(domain.length() - suffix.length(), suffix.length(), suffix) == 0;
    }
    return false;
}


int main() {
    std::ifstream file(filename);
    nlohmann::json j;

    if (file.is_open()) {
        try {
            file >> j;
        } catch (nlohmann::json::parse_error &e) {
            std::cerr << "JSON parse error: " << e.what() << std::endl;
            return 1;
        }
        file.close();

        if (!j.contains("api_token") || !j.contains("domains")) {
            std::cerr << "Missing required fields in JSON (api_token or domains)" << std::endl;
            return 1;
        }
        if (!j["domains"].is_array()) {
            std::cerr << "'domains' field must be an array" << std::endl;
            return 1;
        }

        std::string api_token = j["api_token"].get<std::string>();

        // IPアドレスの取得
        std::string current_ip = get_ip();
        if (current_ip.empty()) {
            std::cerr << "Could not retrieve current IP address." << std::endl;
            return 1;
        }
        std::cout << "Current IP address: " << current_ip << std::endl;

        // 前回保存されたIPと比較
        std::string last_ip = j.contains("ip") ? j["ip"].get<std::string>() : "";
        if (last_ip == current_ip) {
            std::cout << "IP address has not changed. Skipping update." << std::endl;
            return 0;
        }

        // ゾーン情報のキャッシュを管理するオブジェクト（無ければ作成）
        if (!j.contains("zones") || !j["zones"].is_object()) {
            j["zones"] = nlohmann::json::object();
        }

        bool all_success = true;
        bool json_updated = false; // JSONに新しい情報を追記したかどうかのフラグ

        // 取得済みのゾーンリスト（必要になるまでAPIを叩かないように遅延評価するための変数）
        std::vector<std::array<std::string, 2>> fetched_zones;
        bool has_fetched_zones = false;

        for (const auto& domain_element : j["domains"]) {
            std::string target_domain = domain_element.get<std::string>();
            std::string zone_id = "";

            // 1. まずはJSON内の「zones」キャッシュから探す
            for (auto& [zone_name, id_value] : j["zones"].items()) {
                if (is_domain_in_zone(target_domain, zone_name)) {
                    zone_id = id_value.get<std::string>();
                    break;
                }
            }

            // 2. キャッシュに無ければ、Cloudflare APIからゾーンリストを（初回のみ）取得して探す
            if (zone_id.empty()) {
                if (!has_fetched_zones) {
                    std::cout << "Zone cache miss. Fetching zones from Cloudflare API..." << std::endl;
                    fetched_zones = list_zones(api_token);
                    has_fetched_zones = true;
                }

                for (const auto &zone : fetched_zones) {
                    if (is_domain_in_zone(target_domain, zone[0])) {
                        zone_id = zone[1];
                        // 見つかったゾーン情報をJSONキャッシュに記録
                        j["zones"][zone[0]] = zone_id;
                        json_updated = true;
                        break;
                    }
                }
            }

            if (zone_id.empty()) {
                std::cerr << "Matchable zone not found for domain: " << target_domain << std::endl;
                all_success = false;
                continue;
            }

            // 3. DNSレコードの同期処理
            std::vector<std::array<std::string, 2>> existing_records = find_A_record(api_token, zone_id);
            std::string record_id = "";

            for (const auto &record : existing_records) {
                if (record[0] == target_domain) {
                    record_id = record[1];
                    break;
                }
            }

            bool success = false;
            if (!record_id.empty()) {
                std::cout << "Updating existing record for " << target_domain << " (Zone ID: " << zone_id << ")" << std::endl;
                success = update_record(api_token, zone_id, record_id, target_domain, current_ip);
            } else {
                std::cout << "Creating new record for " << target_domain << " (Zone ID: " << zone_id << ")" << std::endl;
                success = create_record(api_token, zone_id, target_domain, current_ip);
            }

            if (!success) {
                all_success = false;
            }
        }

        // すべての同期が成功した場合、またはゾーン情報のキャッシュが更新された場合にJSONに保存
        if (all_success || json_updated) {
            if (all_success) {
                j["ip"] = current_ip;
            }

            std::ofstream out_file(filename);
            if (out_file.is_open()) {
                out_file << j.dump(4);
                out_file.close();
                if (all_success) {
                    std::cout << "All domains updated successfully. Configuration saved." << std::endl;
                } else {
                    std::cout << "Some updates failed, but zone cache was updated and saved." << std::endl;
                }
            } else {
                std::cerr << "Failed to save the updated JSON file." << std::endl;
                return 1;
            }
        } else {
            std::cerr << "Some or all Cloudflare updates failed. IP was not cached." << std::endl;
            return 1;
        }

        return 0;
    } else {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return 1;
    }
}