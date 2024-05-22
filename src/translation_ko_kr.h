

#pragma once
#include "engine/constexpr_containers.h"
#include "strings.h"

namespace strings {
namespace ko_kr {
constexpr CEMap<i18n, const char*, magic_enum::enum_count<i18n>()>
    pre_translation = {{{

        //
        {i18n::ExampleFormattedString, "당신의 사용자 이름은 {username}입니다"},
        // Menu Buttons
        {i18n::Play, "플레이"},
        {i18n::Settings, "설정"},
        {i18n::About, "소개"},
        {i18n::Exit, "종료"},
        //
        {i18n::RoundEndLayer_Countdown, "다음 라운드 시작까지 {time_left}"},

        {i18n::StoreNotEnoughCoins, "코인이 충분하지 않습니다"},
        {i18n::StoreMissingRequired, "필요한 기계가 없습니다"},
        {i18n::StoreStealingMachine, "그 기계를 다시 놓아주세요"},
        {i18n::StoreBalance, "잔액"},
        {i18n::StoreTip, "팁"},

        // In Game
        {i18n::START_GAME, "게임 시작"},
        {i18n::CUSTOMERS_IN_STORE, "모든 고객이 떠날 때까지 닫을 수 없습니다"},
        {i18n::HOLDING_FURNITURE,
         "모든 플레이어가 가구를 놓을 때까지 게임을 시작할 수 없습니다"},
        {i18n::NO_PATH_TO_REGISTER,
         "등록 경로가 없으면 게임을 시작할 수 없습니다"},
        {i18n::BAR_NOT_CLEAN,
         "바가 모두 깨끗해질 때까지 게임을 시작할 수 없습니다"},
        {i18n::FURNITURE_OVERLAPPING,
         "가구가 겹치면 게임을 시작할 수 없습니다"},
        {i18n::ITEMS_IN_SPAWN_AREA,
         "가구가 스폰 지역에 남아 있으면 게임을 시작할 수 없습니다"},
        {i18n::DELETING_NEEDED_ITEM, "필요한 기계를 삭제할 수 없습니다..."},
        {i18n::LOADING, "로딩중..."},
        {i18n::OPEN, "OPEN"},
        {i18n::CLOSING, "CLOSING"},
        {i18n::CLOSED, "CLOSED"},
        {i18n::NEXT_ROUND_COUNTDOWN, "다음 라운드 시작까지"},
        {i18n::CHARACTER_SWITCHER, "캐릭터 스위처"},

        {i18n::PLANNING_CUSTOMERS_COMING, "고객이 오고 있습니다"},

        {i18n::TRIGGERAREA_PURCHASE_FINISH, "제출 및 반환"},

        {i18n::FLOORMARKER_TRASH, "쓰레기"},
        {i18n::FLOORMARKER_NEW_ITEMS, "새로운 항목"},
        {i18n::FLOORMARKER_STORE_PURCHASE, "구입 장소"},

        //
        {i18n::BACK_BUTTON, "뒤로"},

        // Pause Menu
        {i18n::CONTINUE, "계속"},
        {i18n::QUIT, "退出"},

        // Network Stuff
        {i18n::JOIN, "가입"},
        {i18n::HOST, "호스트"},
        {i18n::EDIT, "편집"},
        {i18n::START, "시작"},
        {i18n::DISCONNECT, "연결 끊기"},
        {i18n::USERNAME, "사용자 이름"},
        {i18n::LOCK_IN, "잠금"},
        {i18n::ENTER_IP, "IP 주소 입력"},
        {i18n::LOAD_LAST_IP, "마지막으로 사용한 IP 로드"},
        {i18n::CONNECT, "연결"},

        {i18n::HIDE_IP, "숨기기"},
        {i18n::SHOW_IP, "표시"},
        {i18n::COPY_IP, "복사"},

        // Settings Page
        {i18n::SAFE_ZONE, "안전 구역"},
        {i18n::SHOW_SAFE_BOX, "스트리머 안전 상자 표시"},
        {i18n::ENABLE_PPS, "포스트 프로세싱 셰이더 활성화"},
        {i18n::SNAP_CAMERA, "카메라 스냅"},
        {i18n::MASTER_VOLUME, "마스터 볼륨"},
        {i18n::MUSIC_VOLUME, "음악 볼륨"},
        {i18n::SOUND_VOLUME, "사운드 볼륨"},
        {i18n::RESOLUTION, "해상도"},
        {i18n::THEME, "테마"},
        {i18n::LANGUAGE, "언어"},
        {i18n::FULLSCREEN, "전체 화면?"},

        {i18n::EXIT_AND_SAVE, "저장 및 종료"},
        {i18n::EXIT_NO_SAVE, "저장하지 않고 종료"},
        {i18n::RESET_ALL_SETTINGS, "모든 설정 초기화"},

        {i18n::GENERAL, "일반"},
        {i18n::CONTROLS, "컨트롤"},
        {i18n::KEYBOARD, "키보드"},
        {i18n::GAMEPAD, "게임패드"},

        {i18n::FAKESTRING_CAPS, "ABCDEFGHIJKLMNOPQRSTUVWXYZ "},
        {i18n::FAKESTRING_NAPS, "abcdefghijklmnopqrstuvwxyz "},
        {i18n::FAKESTRING_NUMS, "0123456789_-+=/[];,.<>?:"},

        {i18n::Empty, ""},
        {i18n::InternalError, "내부 오류"},
    }}};

}  // namespace ko_kr
}  // namespace strings
