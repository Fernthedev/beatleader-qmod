#include "HMUI/Touchable.hpp"

#include "questui/shared/QuestUI.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/MainThreadScheduler.hpp"

#include "UnityEngine/Application.hpp"
#include "UnityEngine/GUIUtility.hpp"
#include "UnityEngine/HideFlags.hpp"

#include "Utils/ModConfig.hpp"
#include "Utils/WebUtils.hpp"
#include "API/PlayerController.hpp"

#include "include/UI/PreferencesViewController.hpp"
#include "include/UI/LevelInfoUI.hpp"

#include "main.hpp"

#include <string>

using namespace QuestUI;
using namespace std;

DEFINE_TYPE(BeatLeader, PreferencesViewController);

UnityEngine::UI::Button* logoutButton;
HMUI::InputFieldView* loginField;
HMUI::InputFieldView* passwordField;
UnityEngine::UI::Button* loginButton;
UnityEngine::UI::Button* signupButton;
TMPro::TextMeshProUGUI* label1;
TMPro::TextMeshProUGUI* label2;
TMPro::TextMeshProUGUI* label3;
TMPro::TextMeshProUGUI* errorDescriptionLabel;

HMUI::SimpleTextDropdown* serverDropdown;
UnityEngine::UI::Toggle* saveToggle;

StringW login;
StringW password;

string errorDescription = "";

void UpdateUI(string userID) {
    if (userID.length() > 0) {
        label2->SetText(userID);
        label2->get_gameObject()->SetActive(true);
        label1->get_gameObject()->SetActive(true);
        label3->get_gameObject()->SetActive(false);
        logoutButton->get_gameObject()->SetActive(true);

        loginField->get_gameObject()->SetActive(false);
        passwordField->get_gameObject()->SetActive(false);
        loginButton->get_gameObject()->SetActive(false);
        signupButton->get_gameObject()->SetActive(false);
    } else {
        label2->get_gameObject()->SetActive(false);
        label1->get_gameObject()->SetActive(false);
        label3->get_gameObject()->SetActive(true);
        logoutButton->get_gameObject()->SetActive(false);

        loginField->get_gameObject()->SetActive(true);
        passwordField->get_gameObject()->SetActive(true);
        loginButton->get_gameObject()->SetActive(true);
        signupButton->get_gameObject()->SetActive(true);
    }

    errorDescriptionLabel->SetText(errorDescription);
    if (errorDescription.length() > 0) {
        errorDescriptionLabel->get_gameObject()->SetActive(true);
    } else {
        errorDescriptionLabel->get_gameObject()->SetActive(false);
    }
}

void BeatLeader::PreferencesViewController::DidDeactivate(bool removedFromHierarchy, bool screenSystemDisabling) {
    errorDescription = "";
}

void BeatLeader::PreferencesViewController::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    string userID = PlayerController::currentPlayer != NULL ? PlayerController::currentPlayer->id : "";

    if (firstActivation) {
        this->get_gameObject()->AddComponent<HMUI::Touchable*>();
        UnityEngine::GameObject* container = BeatSaberUI::CreateScrollableSettingsContainer(this->get_transform());

        label1 = ::QuestUI::BeatSaberUI::CreateText(container->get_transform(), "Your id:", false);
        label2 = ::QuestUI::BeatSaberUI::CreateText(container->get_transform(), userID, false);
        logoutButton = ::QuestUI::BeatSaberUI::CreateUIButton(container->get_transform(), "Logout", [](){
            if (!PlayerController::LogOut()) {
                errorDescription = PlayerController::lastErrorDescription;
            } else {
                errorDescription = "";
                UpdateUI("");
            }
        });

        loginField = ::QuestUI::BeatSaberUI::CreateStringSetting(container->get_transform(), "Login", "", [](StringW value) {
            login = value;
        });
        login = PlayerController::platformPlayer != NULL ? PlayerController::platformPlayer->name : "";
        loginField->SetText(login);
        
        passwordField = ::QuestUI::BeatSaberUI::CreateStringSetting(container->get_transform(), "Password", "", [](StringW value) {
            password = value;
        });

        loginButton = ::QuestUI::BeatSaberUI::CreateUIButton(container->get_transform(), "Log in", []() {
            if (!login || !password) {
                errorDescription = "Enter a username and/or password!";
                UpdateUI("");
                return;
            }
            PlayerController::LogIn((string)login, (string)password, [](string userID) { QuestUI::MainThreadScheduler::Schedule([userID] {
                if (userID.length() == 0) {
                    errorDescription = PlayerController::lastErrorDescription;
                } else {
                    errorDescription = "";
                    login = "";
                    loginField->SetText("");
                    password = "";
                    passwordField->SetText("");
                }
                UpdateUI(userID);      
            });});
        });
        signupButton = ::QuestUI::BeatSaberUI::CreateUIButton(container->get_transform(), "Sign up", []() {
            if (!login || !password) {
                errorDescription = "Enter a username and/or password!";
                UpdateUI("");
                return;
            }
            PlayerController::SignUp((string)login, (string)password, [](string userID) { QuestUI::MainThreadScheduler::Schedule([userID] {
                if (userID.length() == 0) {
                    errorDescription = PlayerController::lastErrorDescription;
                } else {
                    errorDescription = "";
                    login = "";
                    loginField->SetText("");
                    password = "";
                    passwordField->SetText("");
                }
                UpdateUI(userID);
            });});
        });
        // serverDropdown = ::QuestUI::BeatSaberUI::CreateDropdown(container->get_transform(), "Server type", getModConfig().ServerType.GetValue(), {"Main", "Test"}, [](string serverType) {
        //     getModConfig().ServerType.SetValue(serverType);
        //     WebUtils::refresh_urls();
        //     LevelInfoUI::resetStars();
        //     PlayerController::Refresh();
        // });
        PlayerController::playerChanged.push_back([](Player* updated) {
            UpdateUI(updated->id);
        });
        saveToggle = AddConfigValueToggle(container->get_transform(), getModConfig().Save);
        errorDescriptionLabel = ::QuestUI::BeatSaberUI::CreateText(container->get_transform(), "", false);
        label3 = ::QuestUI::BeatSaberUI::CreateText(container->get_transform(), "To sign up, enter your login information.\nTo log in, enter your existing account's login information.\nYour account is temporary until at least one score has been posted!\nYou can change your profile picture on the website.", false);
    }

    UpdateUI(userID);
}