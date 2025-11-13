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
              KC_LGUI   , KC_LALT  , MO(1)    , KC_SPC   , _______  ,                                        KC_BSPC  ,LT(2,KC_ENT), RCTL_T(KC_LNG2)   , KC_RALT  , KC_PSCR
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

enum my_layers {
  _MOUSE = 4,
};

enum my_keys {
    KC_SCROLL = QK_USER_0,
};

// CPI
#define MOUSE_MOVE_CPI 6
#define SCROLL_CPI 1

#define MOUSE_MODE_TIMEOUT 1500
#define MOUSE_MODE_MOVE_THRESHOLD 0
#define MOUSE_MODE_TIME_THRESHOLD 10

#define SCROLL_DIVISOR 2

// 状態を管理するグローバル変数
static uint16_t move_start_timer = 0; // 開始用カウンタ
static uint16_t mouse_mode_timer = 0; // タイムアウト用タイマー
static bool is_pressed_scroll = false; // スクロールキー(,)が押されているか
static int16_t x_acc = 0; // X軸アキュムレータ
static int16_t y_acc = 0; // Y軸アキュムレータ
static bool was_scrolling = false; // モード切替検知用
/**
 * @brief 起動後処理
 */
void keyboard_post_init_user(void)
{
    // 起動時にCPIを STARTUP_CPI に設定する
    pointing_device_set_cpi(MOUSE_MOVE_CPI);
}

/**
 * @brief マウスイベントコールバック
 */
report_mouse_t pointing_device_task_kb(report_mouse_t report)
{
    // 1. マウスの移動チェック
    bool is_change_mouse_mode = false;
    bool is_moved_mouse = abs(report.x) > MOUSE_MODE_MOVE_THRESHOLD || abs(report.y) > MOUSE_MODE_MOVE_THRESHOLD;
    bool is_touched_mouse = report.x != 0 || report.y != 0;

    if (is_moved_mouse) { // マウスが動いた
        if (move_start_timer == 0) {
            move_start_timer = timer_read();
        }
    }
    if (is_touched_mouse) {
        is_change_mouse_mode = true;
    } else {
        move_start_timer = 0;
    }

    // 2. スクロールキーが押されているかチェック
    {
        if (is_pressed_scroll != was_scrolling) {
            x_acc = 0;
            y_acc = 0;
            was_scrolling = is_pressed_scroll;
        }

        // 現在の移動量をアキュムレータに加算
        x_acc += report.x;
        y_acc -= report.y; // 縦スクロールは反転

        if (is_pressed_scroll) {
            report.h = x_acc / SCROLL_DIVISOR; // 水平スクロール
            report.v = y_acc / SCROLL_DIVISOR; // 垂直スクロール
            
            x_acc = x_acc % SCROLL_DIVISOR;
            y_acc = y_acc % SCROLL_DIVISOR;
            
            report.x = 0;
            report.y = 0;
        }
    }

    // 3. マウスモードのタイマー管理
    if (is_change_mouse_mode || is_pressed_scroll) {
        // マウスが動いた or スクロール中なら、モードをONにしてタイマーリセット
        if (!IS_LAYER_ON(_MOUSE)) {
            layer_on(_MOUSE);
        }
        mouse_mode_timer = timer_read();
    } else {
        // マウスが動いていない場合 (タイムアウト処理)
        if (move_start_timer == 0) { // 連続移動がリセットされた後でのみタイムアウト判定
            if (mouse_mode_timer == 0 || timer_elapsed(mouse_mode_timer) > MOUSE_MODE_TIMEOUT) {
                // タイムアウトしたら _MOUSE レイヤーをOFFにする
                if (IS_LAYER_ON(_MOUSE)) {
                    layer_off(_MOUSE);
                }
                mouse_mode_timer = 0;
            }
        }
    }

    // 4. 速度調整
    if (IS_LAYER_ON(1)) {
        report.x /= 2;  
        report.y /= 2;
        report.v /= 2;
        report.h /= 2;
    }    


    // 5. 処理済みのレポートを返す
    return report;
}

/**
 * @brief キーが押されるたびに呼ばれる
 */
bool process_record_user(uint16_t keycode, keyrecord_t *record)
{
    // マウスレイヤーがONのときに、指定のキーが押されたかチェック
    if (IS_LAYER_ON(_MOUSE) && record->event.pressed) {
        switch (keycode) {
            case KC_BTN1:       // クリック
            case KC_BTN2:       // 右クリック
            case KC_BTN3:       // 中クリック
            case KC_SCROLL:     // スクロール用
            case LALT(KC_LEFT): // 戻る
            case LALT(KC_RGHT): // 進む
                // マウス関連キーが押されたらタイマーをリセット（モード延長）
                mouse_mode_timer = timer_read();
                break;
            default:
                // マウス関連でないキー入力があったら即終了
                layer_off(_MOUSE);
                mouse_mode_timer = 0;   
                break;
        }
    }
    
    // スクロールキーの処理
    switch (keycode) {
        case KC_SCROLL:
            if (record->event.pressed) {
                if (!IS_LAYER_ON(_MOUSE)) {
                    layer_on(_MOUSE);
                }
                if (!is_pressed_scroll) {
                    pointing_device_set_cpi(SCROLL_CPI);
                    is_pressed_scroll = true;
                }
            } else {
                if (is_pressed_scroll) {
                    pointing_device_set_cpi(MOUSE_MOVE_CPI);
                    is_pressed_scroll = false;
                }
            }
    }

    return true;
}
