
#pragma once
#include "engine/constexpr_containers.h"
#include "strings.h"

namespace strings {
namespace es_la {
constexpr CEMap<i18n, const char*, magic_enum::enum_count<i18n>()>
    pre_translation = {{{
        //
        {i18n::ExampleFormattedString, "Tu nombre de usuario es {username}"},
        // Menu Buttons
        {i18n::Play, "Jugar"},
        {i18n::Settings, "Ajustes"},
        {i18n::About, "Acerca de"},
        {i18n::Exit, "Salir"},
        //
        {i18n::RoundEndLayer_Countdown,
         "Siguiente ronda comenzando en {time_left}"},
        {i18n::StoreNotEnoughCoins, "No hay monedas suficientes"},
        {i18n::StoreMissingRequired, "Máquina requerida ausente"},
        {i18n::StoreStealingMachine, "Por favor, devuelva esa máquina"},
        {i18n::StoreBalance, "Balance"},
        {i18n::StoreTip, "propina"},
        // In Game
        {i18n::START_GAME, "Comenzar juego"},
        {i18n::CUSTOMERS_IN_STORE,
         "No se puede cerrar hasta que todos los clientes se vayan"},
        {i18n::HOLDING_FURNITURE,
         "No se puede comenzar el juego hasta que todos los jugadores suelten "
         "los muebles"},
        {i18n::NO_PATH_TO_REGISTER,
         "No se puede comenzar el juego hasta que haya un camino a una caja "
         "registradora"},
        {i18n::BAR_NOT_CLEAN,
         "No se puede comenzar el juego hasta que su barra esté limpia"},
        {i18n::FURNITURE_OVERLAPPING,
         "No se puede comenzar el juego si tiene muebles superpuestos"},
        {i18n::ITEMS_IN_SPAWN_AREA,
         "No se puede comenzar el juego si tiene muebles en el área de spawn"},
        {i18n::DELETING_NEEDED_ITEM,
         "No se puede eliminar máquinas necesarias para recetas..."},
        {i18n::LOADING, "Cargando..."},
        {i18n::OPEN, "ABIERTO"},
        {i18n::CLOSING, "CERRANDO"},
        {i18n::CLOSED, "CERRADO"},
        {i18n::NEXT_ROUND_COUNTDOWN, "Siguiente ronda comenzando en"},
        {i18n::CHARACTER_SWITCHER, "Selector de personajes"},
        {i18n::PLANNING_CUSTOMERS_COMING, "Clientes llegando"},
        {i18n::ROUND_DAY, "Día"},
        {i18n::TRIGGERAREA_PURCHASE_FINISH, "Enviar y regresar"},
        {i18n::FLOORMARKER_TRASH, "Basura"},
        {i18n::FLOORMARKER_NEW_ITEMS, "Artículos nuevos"},
        {i18n::FLOORMARKER_STORE_PURCHASE, "Lugar de compra"},
        //
        {i18n::BACK_BUTTON, "Atrás"},
        // Pause Menu
        {i18n::CONTINUE, "Continuar"},
        {i18n::QUIT, "Salir"},
        // Network Stuff
        {i18n::JOIN, "Unirse"},
        {i18n::HOST, "Anfitrión"},
        {i18n::EDIT, "Editar"},
        {i18n::START, "Iniciar"},
        {i18n::DISCONNECT, "Desconectar"},
        {i18n::USERNAME, "Nombre de usuario"},
        {i18n::LOCK_IN, "Bloquear"},
        {i18n::ENTER_IP, "Ingresar dirección IP"},
        {i18n::LOAD_LAST_IP, "Cargar última dirección IP utilizada"},
        {i18n::CONNECT, "Conectar"},
        {i18n::HIDE_IP, "Ocultar"},
        {i18n::SHOW_IP, "Mostrar"},
        {i18n::COPY_IP, "Copiar"},
        // Settings Page
        {i18n::SAFE_ZONE, "zona segura"},
        {i18n::SHOW_SAFE_BOX, "Mostrar caja de streamer segura"},
        {i18n::ENABLE_PPS, "Habilitar sombras posteriores"},
        {i18n::SNAP_CAMERA, "Cámara de fotos"},
        {i18n::MASTER_VOLUME, "Volumen principal"},
        {i18n::MUSIC_VOLUME, "Volumen de música"},
        {i18n::SOUND_VOLUME, "Volumen de efectos"},
        {i18n::RESOLUTION, "Resolución"},
        {i18n::THEME, "Tema"},
        {i18n::LANGUAGE, "Idioma"},
        {i18n::FULLSCREEN, "Pantalla completa"},
        {i18n::EXIT_AND_SAVE, "Guardar y salir"},
        {i18n::EXIT_NO_SAVE, "Salir sin guardar"},
        {i18n::RESET_ALL_SETTINGS, "Restablecer todos los ajustes"},
        {i18n::GENERAL, "General"},
        {i18n::CONTROLS, "Controles"},
        {i18n::KEYBOARD, "Teclado"},
        {i18n::GAMEPAD, "Gamepad"},

        {i18n::FAKESTRING_CAPS, "ABCDEFGHIJKLMNOPQRSTUVWXYZ "},
        {i18n::FAKESTRING_NAPS, "abcdefghijklmnopqrstuvwxyz "},
        {i18n::FAKESTRING_NUMS, "0123456789_-+=/[];,.<>?:"},

        {i18n::Empty, ""},
        {i18n::InternalError, "Internal Error"},

    }}};

}  // namespace es_la
}  // namespace strings
