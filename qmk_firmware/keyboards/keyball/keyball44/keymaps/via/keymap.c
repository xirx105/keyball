/*
Copyright 2022 @Yowkees
Copyright 2022 MURAOKA Taro (aka KoRoN, @kaoriya)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include QMK_KEYBOARD_H

#include "quantum.h"

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  // keymap for default (VIA)
  [0] = LAYOUT_universal(
    KC_ESC   , KC_Q     , KC_W     , KC_E     , KC_R     , KC_T     ,                                        KC_Y     , KC_U     , KC_I     , KC_O     , KC_P     , KC_DEL   ,
    KC_TAB   , KC_A     , KC_S     , KC_D     , KC_F     , KC_G     ,                                        KC_H     , KC_J     , KC_K     , KC_L     , KC_SCLN  , S(KC_7)  ,
    KC_LSFT  , KC_Z     , KC_X     , KC_C     , KC_V     , KC_B     ,                                        KC_N     , KC_M     , KC_COMM  , KC_DOT   , KC_SLSH  , KC_INT1  ,
              KC_LALT,KC_LGUI,LCTL_T(KC_LNG2)     ,LT(1,KC_SPC),LT(3,KC_LNG1),                  KC_BSPC,LT(2,KC_ENT), RCTL_T(KC_LNG2),     KC_RALT  , KC_PSCR
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

#    include "lib/oledkit/oledkit.h"

void oledkit_render_info_user(void) {
    keyball_oled_render_keyinfo();
    keyball_oled_render_ballinfo();
    keyball_oled_render_layerinfo();
}
#endif

/* ----- ヨー回転による自動レイヤー切り替え機能 ここから ----- */

// 感度設定 (値を小さくすると敏感になる)
#define YAW_SCROLL_THRESHOLD 400
#define YAW_SCROLL_SCALE_BASE 50
// タイムアウト (ミリ秒)
#define YAW_SCROLL_TIMEOUT 300

// 状態を保存する変数
static int32_t cumulative_rotation = 0;
static int16_t last_x = 0;
static int16_t last_y = 0;
static uint32_t last_time = 0;
static bool yaw_scroll_layer_active = false; // ヨー回転で切り替わったかのフラグ

report_mouse_t pointing_device_task_user(report_mouse_t mouse_report) {
    int16_t x = mouse_report.x;
    int16_t y = mouse_report.y;

    // --- タイムアウト処理 ---
    if (timer_elapsed32(last_time) > YAW_SCROLL_TIMEOUT) {
        cumulative_rotation = 0;
        last_x = 0;
        last_y = 0;
        // ヨー回転でレイヤー3に入っていた場合のみ、自動でレイヤーをオフにする
        if (yaw_scroll_layer_active) {
            layer_off(3);
            yaw_scroll_layer_active = false;
        }
    }

    // --- 不感帯（デッドゾーン）処理 ---
    if (abs(x) < 2 && abs(y) < 2) {
        // 動きが止まっている時はタイマーをリセットしない（タイムアウトさせるため）
        return mouse_report;
    }

    // --- ここから下は、ボールが確実に動いている時の処理 ---
    // ボールが動いているので、タイムアウトタイマーをリセット
    last_time = timer_read32();

    // 「外積」を計算して回転量を蓄積
    int32_t cross_product = (int32_t)x * last_y - (int32_t)y * last_x;
    cumulative_rotation += cross_product;

    // 状態を更新
    last_x = x;
    last_y = y;

    // 蓄積された回転量が閾値を超えたら、
    // スクロールイベントを「生成」し、レイヤー3を「有効化」する
    if (abs(cumulative_rotation) > YAW_SCROLL_THRESHOLD) {
        if (!yaw_scroll_layer_active) {
            layer_on(3); // レイヤー3をオンにする（OLED表示用）
            yaw_scroll_layer_active = true; 
        }
        
        // ヨー回転の向きに応じてスクロール方向を決定
        int32 scale = (cumulative_rotation - YAW_SCROLL_THRESHOLD) / YAW_SCROLL_SCALE_BASE + 1;
        if (cumulative_rotation > 0) {
            mouse_report.v = scale; // 垂直スクロール（下）
        } else {
            mouse_report.v = -scale; // 垂直スクロール（上）
        }

        // 元のポインタ移動はキャンセルする
        mouse_report.x = 0;
        mouse_report.y = 0;
        
        cumulative_rotation = 0; // 蓄積量をリセット
    }

    return mouse_report;
}

/* ----- ヨー回転による自動レイヤー切り替え機能 ここまで ----- */
