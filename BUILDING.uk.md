[English](BUILDING.md) | **[Українська](BUILDING.uk.md)** | [Русский](BUILDING.ru.md)

# Збірка з вихідного коду

## Що потрібно

Потрібен **devkitPro** з тулчейном для Switch (`devkitA64` + `libnx`), а
також **libjpeg-turbo** для Switch (потрібна для декодування JPEG-іконок
ігор, вбудованих у `NsApplicationControlData`).

### Windows

1. Завантажте та запустіть [графічний інсталятор devkitPro](https://github.com/devkitPro/installer/releases)
   для Windows. Коли він запитає, які платформи встановити, обов'язково
   поставте галочку на **Switch development**.
2. Після завершення відкрийте **Пуск → devkitPro → MSYS2** (це вбудована
   оболонка MSYS2 з уже налаштованими репозиторіями devkitPro — використовуйте
   саме її, а не звичайні `cmd`/PowerShell, для всіх кроків нижче).
3. У цьому вікні MSYS2 встановіть тулчейн для Switch і libjpeg-turbo:
   ```bash
   pacman -S switch-dev switch-libjpeg-turbo
   ```
4. Закрийте і знову відкрийте вікно MSYS2 один раз (щоб підхопилась змінна
   середовища `DEVKITPRO`, яку виставляє інсталятор), тоді перевірте її:
   ```bash
   echo $DEVKITPRO
   ```
   Має вивести щось на кшталт `/c/devkitPro`. Якщо порожньо — інсталятор
   завершився некоректно, запустіть його ще раз.
5. Далі виконуйте кроки **Збірка** нижче з того самого вікна MSYS2.

> **Примітка:** `git` не входить до складу MSYS2 devkitPro за замовчуванням.
> Можна або встановити його там (`pacman -S git`), або просто завантажити
> репозиторій ZIP-архівом з GitHub (**Code → Download ZIP**), розпакувати
> його і зайти в цю папку з MSYS2 замість `git clone`.

### Linux / macOS

Встановіть через пакетний менеджер [devkitPro pacman](https://devkitpro.org/wiki/devkitPro_pacman):
```bash
sudo dkp-pacman -S switch-dev switch-libjpeg-turbo
```
Переконайтесь, що змінна `DEVKITPRO` виставлена в середовищі (інсталятор
робить це сам; перевірте через `echo $DEVKITPRO`, за потреби перелогіньтесь
одразу після встановлення, якщо вона порожня).

## Збірка

```bash
git clone https://github.com/Rexead/GameTime-Tracker.git
cd GameTime-Tracker
make
```

Це створить `GameTime-Tracker.nro` в корені проєкту (назва береться з назви
папки — перейменуйте папку, якщо хочете інше ім'я `.nro`).

`make clean` видаляє папку `build/` та всі артефакти збірки.

## Структура проєкту

```
GameTime-Tracker/
├── source/          # .cpp файли (компілюються Makefile)
│   ├── main.cpp         # основний цикл, обробка вводу, стан екранів
│   ├── game_loader.cpp  # запити до NS/PDM, декодування JPEG-іконок, фоновий потік завантаження
│   ├── renderer.cpp     # відмальовка у framebuffer (список, деталі, шкала завантаження, растровий шрифт)
│   └── utils.cpp        # форматування часу гри
├── include/
│   └── gametime.h       # спільні структури, константи інтерфейсу, оголошення функцій
└── Makefile
```

## Встановлення на консоль

Дивіться розділ [Встановлення](README.uk.md#встановлення) в README —
той самий `.nro`, який ви зібрали тут, кидається на SD-карту так само,
як і збірка з Releases.
