# esphome-components

Custom ESPHome external components.

## Components

### daikin_arc472a66

ダイキン ARC472A66 リモコン(赤外線)用の climate コンポーネントです。実機リモコンを
M5Stack Atom Lite + Unit IR で解析した結果 (`PROTOCOL_NOTES.md` 参照) を反映しています。

- climate標準機能: 電源・モード(冷房/除湿/暖房/送風/快適エコ自動)・温度・風量
  (自動/しずか/1〜5段階)・湿度(除湿時)・4種のswing_mode・preset(パワフル/おやすみ)
- オートメーションアクション: 快適エコ自動⇔ビズ自動の切替・快適エコ自動の温度オフセット・
  暖房時の加湿・風向上下(6段階+揺らぎ+スイング+センサー自動)・風向左右(7段階+ワイド+
  スイング+センサー自動)・センサー風向(OFF/エリア/スポット)・サーキュレーション・
  Streamer(空清)・内部クリーン/フィルター掃除/お知らせの単発送信

#### 使い方

```yaml
external_components:
  - source: github://haoling/esphome-components
    components: [daikin_arc472a66]

remote_transmitter:
  id: ir_tx
  pin: GPIO4
  carrier_duty_percent: 50%

remote_receiver:
  id: ir_rx
  pin: GPIO32

climate:
  - platform: daikin_arc472a66
    name: "Daikin AC"
    id: daikin_ac
    receiver_id: ir_rx
    transmitter_id: ir_tx
    power_off_internal_clean: false  # デフォルトfalse。trueにすると停止時に内部クリーン専用フレームを追加送信するが、
                                      # エアコン本体の自律的な内部クリーン開始をキャンセルしてしまう不具合が実機で確認されている(要注意)

# 標準のclimateサービス(climate.control)に加えて、
# 以下のカスタムアクションでARC472A66固有の機能を操作できる
button:
  - platform: template
    name: "Powerful"
    on_press:
      - climate.control:
          id: daikin_ac
          preset: BOOST
  - platform: template
    name: "内部クリーン開始"
    on_press:
      - climate.daikin_arc472a66.start_internal_clean: daikin_ac
  - platform: template
    name: "フィルター掃除"
    on_press:
      - climate.daikin_arc472a66.start_filter_clean: daikin_ac
  - platform: template
    name: "風向スイング(上下)"
    on_press:
      - climate.daikin_arc472a66.set_vertical_vane:
          id: daikin_ac
          vane: SWING
  - platform: template
    name: "サーキュレーションON"
    on_press:
      - climate.daikin_arc472a66.set_circulation:
          id: daikin_ac
          enabled: true
  - platform: template
    name: "快適エコ自動 +1.0℃"
    on_press:
      - climate.daikin_arc472a66.set_comfort_offset:
          id: daikin_ac
          offset: 1.0
```

#### オートメーションアクション一覧

| アクション | パラメータ | 説明 |
|---|---|---|
| `climate.daikin_arc472a66.set_comfort_mode` | `mode`: `COMFORT` \| `BIZ` | 快適エコ自動⇔ビズ自動 |
| `climate.daikin_arc472a66.set_comfort_offset` | `offset`: -1.0〜2.0(0.5刻み) | 快適エコ自動の温度オフセット |
| `climate.daikin_arc472a66.set_humidify` | `enabled`: bool | 暖房モードの加湿ON/OFF |
| `climate.daikin_arc472a66.set_vertical_vane` | `vane`: `POSITION_1`〜`POSITION_6` \| `SWING` \| `SWING_STEP` \| `AUTO` | 風向上下 |
| `climate.daikin_arc472a66.set_horizontal_vane` | `vane`: `WIDE` \| `POSITION_1`〜`POSITION_7` \| `SWING` \| `AUTO` | 風向左右 |
| `climate.daikin_arc472a66.set_sensor_vane` | `mode`: `OFF` \| `AREA` \| `SPOT` | センサー風向 |
| `climate.daikin_arc472a66.set_circulation` | `enabled`: bool | サーキュレーション |
| `climate.daikin_arc472a66.set_streamer` | `enabled`: bool | Streamer(空清) |
| `climate.daikin_arc472a66.start_internal_clean` | (id直接指定) | 内部クリーン単発送信 |
| `climate.daikin_arc472a66.start_filter_clean` | (id直接指定) | フィルター掃除単発送信 |
| `climate.daikin_arc472a66.send_inquiry` | (id直接指定) | お知らせ(状態問い合わせ)単発送信 |

風量の1〜5段階は標準の`fan_mode`ではなく`custom_fan_mode: "1"`〜`"5"`として公開される
(`auto`/`quiet`は標準の`fan_mode`)。パワフル/おやすみは標準の`preset`
(`boost`/`sleep`/`none`)として公開される。

### nec_sealing_light

NEC赤外線リモコンで操作するシーリングライト用のライトコンポーネントです。
ON/OFF・輝度調整（6段階）・常夜灯に対応しています。

#### 対応コマンド

| 操作 | アドレス | コマンド |
|------|----------|----------|
| 全灯 | 0x6D82 | 0x59A6 |
| 消灯 | 0x6D82 | 0x41BE |
| 常夜灯 | 0x6D82 | 0x43BC |
| 明るく | 0x6D82 | 0x45BA |
| 暗く | 0x6D82 | 0x44BB |

#### 使い方

```yaml
external_components:
  - source: github://haoling/esphome-components
    components: [nec_sealing_light]

remote_transmitter:
  id: irsend
  pin: GPIO4          # 赤外線LEDを接続したピン
  carrier_duty_percent: 50%

light:
  - platform: nec_sealing_light
    transmitter_id: irsend
    name: "Sealing Light"
    id: sealing_light
    restore_mode: RESTORE_DEFAULT_OFF
    default_transition_length: 0s
    gamma_correct: 1.0
    on_turn_on:
      - light.turn_on:
          id: sealing_light
          brightness: 100%
```

#### 輝度の段階

輝度スライダーの値は以下の6段階にスナップされます。

| 設定値 | 実際の輝度 |
|--------|-----------|
| 0〜9% | 常夜灯 (5%) |
| 10〜29% | 20% |
| 30〜49% | 40% |
| 50〜69% | 60% |
| 70〜89% | 80% |
| 90〜100% | 全灯 (100%) |
