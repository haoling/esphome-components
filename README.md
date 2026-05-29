# esphome-components

Custom ESPHome external components.

## Components

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
    lights:
      - name: "Sealing Light"
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
