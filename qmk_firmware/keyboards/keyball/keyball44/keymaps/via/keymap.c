/*
Copyright 2022 @Yowkees
Copyright 2022 MURAOKA Taro (aka KoRoN, @kｃ
*/

#include QMK_KEYBOARD_H

#include "quantum.h"

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  // keymap for default (VIA)
  [0] = LAYOUT_universal(
    KC_TAB   , KC_Q     , KC_W     , KC_E     , KC_R     , KC_T     ,                                        KC_Y     , KC_U     , KC_I     , KC_O     , KC_P     , KC_DEL   ,
    KC_LCTL  , KC_A     , KC_S     , KC_D     , KC_F     , KC_G     ,                                        KC_H     , KC_J     , KC_K     , KC_L     , KC_SCLN  , S(KC_7)  ,
    KC_LSFT  , KC_Z     , KC_X     , KC_C     , KC_V     , KC_B     ,                                        KC_N     , KC_M     , KC_COMM  , KC_DOT   , KC_SLSH  , KC_INT1  ,
              KC_LGUI   , KC_LALT  , MO(1)    , KC_SPC   , _______  ,                  KC_BSPC,LT(2,KC_ENT), RCTL_T(KC_LNG2),     KC_RALT  , KC_PSCR
  ),

  [1] = LAYOUT_universal(
    SSNP_FRE ,  KC_F1   , KC_F2    , KC_F3   , KC_F4    , KC_F5    ,                                         KC_F6    , KC_F7    , KC_F8    , KC_F9    , KC_F10   , KC_F11   ,
    SSNP_VRT ,  _______ , _______  , KC_UP   , KC_ENT   , KC_DEL   ,                                         KC_PGUP  , KC_BTN1  , KC_UP    , KC_BTN2  , KC_BTN3  , KC_F12   ,
    SSNP_HOR ,  _______ , KC_LEFT  , KC_DOWN , KC_RGHT  , KC_BSPC  ,                                         KC_PGDN  , KC_LEFT  , KC_DOWN  , KC_RGHT  , _______  , _______  ,
                  _______  , _______ , _______  ,         _______  , _______  ,                   _______  , _______  , _______       , _______  , _______
  ),

  [2] = LAYOUT_universal(
    _______  ,S(KC_QUOT), KC_7     , KC_8    , KC_9     , S(KC_8)  ,                                         S(KC_9)  , S(KC_1)  , S(KC_6)  , KC_LBRC  , S(KC_4)  , _______  ,
    _______  ,S(KC_SCLN), KC_4     , KC_5    , KC_6     , KC_RBRC  ,                                         KC_NUHS  , KC_MINS  , S(KC_EQL), S(KC_3)  , KC_QUOT  , S(KC_2)  ,
    _______  ,S(KC_MINS), KC_1     , KC_2    , KC_3     ,S(KC_RBRC),                                        S(KC_NUHS),S(KC_INT1), KC_EQL   ,S(KC_LBRC),S(KC_SLSH),S(KC_INT3),
                  KC_0     , KC_DOT  , _______  ,         _______  , _______  ,                   KC_DEL   , _______  , _______       , _______  , _______
  ),

  [3] = LAYOUT_universal(
    RGB_TOG  , AML_TO   , AML_I50  , AML_D50  , _______  , _______  ,                                        RGB_M_P  , RGB_M_B  , RGB_M_R  , RGB_M_SW , RGB_M_SN , RGB_M_K  ,
    RGB_MOD  , RGB_HUI  , RGB_SAI  , RGB_VAI  , _______  , SCRL_DVI ,                                        RGB_M_X  , RGB_M_G  , RGB_M_T  , RGB_M_TW , _______  , _______  ,
    RGB_RMOD , RGB_HUD  , RGB_SAD  , RGB_VAD  , _______  , SCRL_DVD ,                                        CPI_D1K  , CPI_D100 , CPI_I100 , CPI_I1K  , _______  , KBC_SAVE ,
                  QK_BOOT  , KBC_RST  , _______  ,        _______  , _______  ,                   _______  , _______  , _______       , KBC_RST  , QK_BOOT
  ),
};
// clang-format on

#ifdef OLED_ENABLE

#include "lib/oledkit/oledkit.h"

void oledkit_render_info_user(void) {
    keyball_oled_render_keyinfo();
    keyball_oled_render_ballinfo();
    keyball_oled_render_layerinfo();
}
#endif

// マウスモードが自動解除されるまでの時間 (ms)
#define MOUSE_MODE_TIMEOUT 3000

enum my_layers {
  _MOUSE = 3,
};

enum my_keycodes {
  MOUSESCRL = SAFE_RANGE,
};

// 状態を管理するグローバル変数
static uint16_t mouse_mode_timer; // タイムアウト用タイマー
static bool scroll_key_pressed = false; // スクロールキー(,)が押されているか

// スクロール速度（値が大きいほど遅くなる）
#define SCROLL_DIVISOR 4

/**
 * @brief マウスが動くたびに呼ばれる (KeyBallドライバ対応版)
 */
report_mouse_t pointing_device_task_kb(report_mouse_t report) {
    // 1. レポートは引数で渡される (get_report() は不要)

    bool mouse_moved = (report.x != 0 || report.y != 0);

    // 2. スクロールキー(,)が押されているかチェック
    if (scroll_key_pressed) {
        // マウスのXY移動を、スクロール(V:垂直, H:水平)に変換
        report.v = report.y / SCROLL_DIVISOR;
        report.h = report.x / SCROLL_DIVISOR;
        
        // 本来のカーソル移動はキャンセル(0)する
        report.x = 0;
        report.y = 0;
    }

    // 3. マウスモードのタイマー管理
    if (mouse_moved || scroll_key_pressed) {
        // マウスが動いた or スクロール中なら、モードをONにしてタイマーリセット
        layer_on(_MOUSE);
        mouse_mode_timer = timer_read();
    } else {
        // マウスが動いていない場合
        if (timer_elapsed(mouse_mode_timer) > MOUSE_MODE_TIMEOUT) {
            // タイムアウトしたら _MOUSE レイヤーをOFFにする
            layer_off(_MOUSE);
        }
    }

    // 4. 処理済みのレポートを返す (set_report() は不要)
    return report;
}

/**
 * @brief キーが押されるたびに呼ばれる
 */
bool process_record_user(uint16_t keycode, keyrecord_t *record) {

    // マウスレイヤーがONのときに、指定のキーが押されたかチェック
    if (IS_LAYER_ON(_MOUSE) && record->event.pressed) {
        switch (keycode) {
            case KC_BTN1:     // J
            case KC_BTN2:     // K
            case KC_BTN3:     // L
            case KC_WWW_BACK: // M
            // ↓↓↓ ★ここを KC_WFWD に変更
            case KC_WFWD:     // 。(KC_WWW_FWD から変更)
                // マウス関連キーが押されたらタイマーをリセット（モード延長）
                mouse_mode_timer = timer_read();
                break;
        }
    }
    
    // スクロールキー(,)の処理
    switch (keycode) {
        case MOUSESCRL:
            if (record->event.pressed) {
                scroll_key_pressed = true;
                // スクロール開始時もモードをONにし、タイマーをリセット
                layer_on(_MOUSE);
                mouse_mode_timer = timer_read();
            } else {
                scroll_key_pressed = false;
            }
            return false; // 「、」キーの通常の入力をブロック
    }

    return true; // 他のキーは通常通り処理
}
