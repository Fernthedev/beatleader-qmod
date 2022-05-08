#include "include/Utils/ReplaySynchronizer.hpp"
#include "include/Utils/FileManager.hpp"
#include "include/Utils/WebUtils.hpp"
#include "include/Utils/ReplayManager.hpp"
#include "include/API/PlayerController.hpp"
#include "include/Models/Replay.hpp"

#include "main.hpp"

#include "beatsaber-hook/shared/rapidjson/include/rapidjson/filewritestream.h"
#include "beatsaber-hook/shared/rapidjson/include/rapidjson/filereadstream.h"
#include "beatsaber-hook/shared/config/rapidjson-utils.hpp"

#include <filesystem>

ReplaySynchronizer::ReplaySynchronizer() noexcept
{
    if (PlayerController::currentPlayer == std::nullopt) return;

    getLogger().info("Synchronizer init");

    ReplaySynchronizer* self = this;
    self->statuses = std::make_shared<rapidjson::Document>();
    std::thread(
            [statuses = self->statuses] {
        string directory = getDataDir(modInfo);
        filesystem::create_directories(directory);

        FILE *fp = fopen((directory + "sync.json").c_str(), "r");
        if (fp != NULL) {
            char buf[0XFFFF];
            FileReadStream input(fp, buf, sizeof(buf));
            statuses->ParseStream(input);
            fclose(fp);
        }

        if (!statuses->HasParseError() && statuses->IsObject()) {
            statuses->SetObject();
        }

        DIR *dir;
        string dirName = directory + "replays/";
        if ((dir = opendir(dirName.c_str())) != NULL) {
            Process(dir, dirName, statuses);
        }
    });
}

void ReplaySynchronizer::updateStatus(string path, ReplayStatus status, rapidjson::Document &doc) {
    doc.AddMember(rapidjson::StringRef(path), status, doc.GetAllocator());
    Save(doc);
}

void ReplaySynchronizer::Save(rapidjson::Document const &doc) {
    string directory = getDataDir(modInfo);
    FILE* fp = fopen((directory + "sync.json").c_str(), "w");
 
    char writeBuffer[65536];
    FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));
    
    Writer<FileWriteStream> writer(os);
    doc.Accept(writer);
    
    fclose(fp);
}

void ReplaySynchronizer::Process(DIR *dir, string dirName, shared_ptr<Document> docPtr) {
    struct dirent *ent;

    if ((ent = readdir(dir)) == NULL || PlayerController::currentPlayer == std::nullopt) {
        closedir(dir);
        return;
    }

    auto& doc = *docPtr;

    string filename = ent->d_name;
    string filePath = dirName + filename;
    getLogger().info("Processing %s", filename.c_str());
    if (doc.HasMember(filePath)) {
        if (ReplayStatus::topost == (ReplayStatus)doc[filePath].GetInt()) {
            if (PlayerController::currentPlayer != std::nullopt) {
                ReplayManager::TryPostReplay(filePath, 0, [filePath, dir, dirName, docPtr](ReplayUploadStatus finished, string_view description, float progress, int status) {
                    auto& doc = *docPtr;
                    if (static_cast<bool>(finished)) {
                        if (status == 200) {
                            doc[filePath].SetInt((int)ReplayStatus::uptodate);
                        } else if (status >= 400 && status < 500) {
                            doc[filePath].SetInt((int)ReplayStatus::shouldnotpost);
                        }
                        Save(doc);
                        Process(dir, dirName, docPtr);
                    }
                });
            }
        } else {
            Process(dir, dirName, docPtr);
        }
    } else {
        auto info = FileManager::ReadInfo(filePath);
        if (info == std::nullopt || info->failTime > 0.001 || info->speed > 0.001 || (PlayerController::currentPlayer != std::nullopt && info->playerID != PlayerController::currentPlayer->id)) {
            updateStatus(filePath, ReplayStatus::shouldnotpost, doc);
            Process(dir, dirName, docPtr);
        } else {
            string url = WebUtils::API_URL + "player/" + PlayerController::currentPlayer->id + "/scorevalue/" + info->hash + "/" + info->difficulty + "/" + info->mode;

            WebUtils::GetJSONAsync(url, [info = std::move(*info), docPtr, dir, dirName, filePath](long status, bool error, rapidjson::Document const& result){
                int score = result.GetInt();
                getLogger().info("Get score %i", score);
                if ((int)((float)info.score * ReplayManager::GetTotalMultiplier(info.modifiers)) > score) {
                    ReplayManager::TryPostReplay(filePath, 0, [filePath, dir, dirName, docPtr](ReplayUploadStatus finished, string_view description, float progress, int status) {
                        if (static_cast<bool>(finished)) {
                            if (status == 200) {
                                updateStatus(filePath, ReplayStatus::uptodate, *docPtr);
                            } else if (status >= 400 && status < 500) {
                                updateStatus(filePath, ReplayStatus::shouldnotpost, *docPtr);
                            }
                            Process(dir, dirName, docPtr);
                        }
                    });
                } else {
                    updateStatus(filePath, ReplayStatus::uptodate, *docPtr);
                    Process(dir, dirName, docPtr);
                }
            });
        }
    }
}