#include "HMUI/Touchable.hpp"

#include "questui/shared/QuestUI.hpp"
#include "questui/shared/BeatSaberUI.hpp"

#include "UnityEngine/Application.hpp"
#include "UnityEngine/GUIUtility.hpp"
#include "UnityEngine/HideFlags.hpp"

#include "Utils/WebUtils.hpp"
#include "API/PlayerController.hpp"
#include "main.hpp"

#include <string>

using namespace QuestUI;
using namespace std;

UnityEngine::UI::Button* logoutButton;
HMUI::InputFieldView* loginField;
HMUI::InputFieldView* passwordField;
UnityEngine::UI::Button* loginButton;
UnityEngine::UI::Button* signupButton;
TMPro::TextMeshProUGUI* label1;
TMPro::TextMeshProUGUI* label2;
TMPro::TextMeshProUGUI* errorDescriptionLabel;

string login = "";
string password = "";

string errorDescription = "";

void UpdateUI(string userID) {
    if (userID.length() > 0) {
        label2->SetText(il2cpp_utils::createcsstr(userID));
        label2->get_gameObject()->SetActive(true);
        label1->get_gameObject()->SetActive(true);
        logoutButton->get_gameObject()->SetActive(true);

        loginField->get_gameObject()->SetActive(false);
        passwordField->get_gameObject()->SetActive(false);
        loginButton->get_gameObject()->SetActive(false);
        signupButton->get_gameObject()->SetActive(false);
    } else {
        label2->get_gameObject()->SetActive(false);
        label1->get_gameObject()->SetActive(false);
        logoutButton->get_gameObject()->SetActive(false);

        loginField->get_gameObject()->SetActive(true);
        passwordField->get_gameObject()->SetActive(true);
        loginButton->get_gameObject()->SetActive(true);
        signupButton->get_gameObject()->SetActive(true);
    }

    errorDescriptionLabel->SetText(il2cpp_utils::createcsstr(errorDescription));

    if (errorDescription.length() > 0) {
        errorDescriptionLabel->get_gameObject()->SetActive(true);
    } else {
        errorDescriptionLabel->get_gameObject()->SetActive(false);
    }
}

void DidActivate(HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    string userID = PlayerController::currentPlayer != NULL ? PlayerController::currentPlayer->id : "";

    if (firstActivation) {
        self->get_gameObject()->AddComponent<HMUI::Touchable*>();
        UnityEngine::GameObject* container = BeatSaberUI::CreateScrollableSettingsContainer(self->get_transform());

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

        loginField = ::QuestUI::BeatSaberUI::CreateStringSetting(container->get_transform(), "Login", "", [](std::string_view value) {
            login = value;
        });
        login = PlayerController::platformPlayer != NULL ? PlayerController::platformPlayer->name : "";
        loginField->SetText(il2cpp_utils::createcsstr(login));
        passwordField = ::QuestUI::BeatSaberUI::CreateStringSetting(container->get_transform(), "Password", "", [](std::string_view value) {
            password = value;
        });
        loginButton = ::QuestUI::BeatSaberUI::CreateUIButton(container->get_transform(), "Log in", []() {
            string userID = PlayerController::LogIn(login, password);
            if (userID.length() == 0) {
                errorDescription = PlayerController::lastErrorDescription;
            } else {
                errorDescription = "";
                login = "";
                loginField->SetText(il2cpp_utils::createcsstr(""));
                password = "";
                passwordField->SetText(il2cpp_utils::createcsstr(""));
            }
            UpdateUI(userID);
        });
        signupButton = ::QuestUI::BeatSaberUI::CreateUIButton(container->get_transform(), "Sign up", []() {
            string userID = PlayerController::SignUp(login, password);
            if (userID.length() == 0) {
                errorDescription = PlayerController::lastErrorDescription;
            } else {
                errorDescription = "";
                login = "";
                loginField->SetText(il2cpp_utils::createcsstr(""));
                password = "";
                passwordField->SetText(il2cpp_utils::createcsstr(""));
            }
            UpdateUI(userID);
        });
        errorDescriptionLabel = ::QuestUI::BeatSaberUI::CreateText(container->get_transform(), "", false);
    }

    UpdateUI(userID);
}