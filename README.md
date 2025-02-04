## About
A [sourcemod](https://github.com/alliedmodders/sourcemod) extension. Load the specified mode through the configuration file. 

## Requirements:
- Sourcemod 1.11 or later

## Description
- `-modename`: Command line option. If set, the extension will auto load the specified mode from **modeloader.cfg** (see below) at startup.
- `sm_modeloader`: Server console command. Manually loads a specific mode. Usually used when changing modes in the middle of a game.
- `sm_modeloader_reloadmap`: Server console command, reload the current map. Usually added to the end of **settings.cfg**.
- `sm_modeloader_execfile`: Server console command, parses the specified file and executes it line by line.
- `sm_modecvar`: Server console command, Locks cvar value and auto restores the default value when the mode change.
- `sm_modecvar_del`: Server console command, Remove cvar added by **sm_modecvar**.
- `sv_modeloader_name`: Console variable. Tracks or gets the current mode name. Auto set when mode change.

## modeloader.cfg
Path: `cfg/modeloader/modeloader.cfg`

```vdf
"modeloader"
{	
    // Any key name. This is not used by the extension. 
    "35006" 
    {
        // Unique value, used by `-modename` and `sm_modeloader`
        "modename"      "MyModeName"

        // Path to the plugin list file to load.
        // Multiple files are separated using `;`
        "plugins_cfg"   "path/plugins1.cfg;path/plugins2.cfg"

        // Path to the setting file to load, such as cvar, cmd, etc.
        "settings_cfg"  "path/settings1.cfg;path/settings2.cfg"
    }

    // Adding other modes.
    // ...
}
```

See the `example` Folder for usage examples.

## Build manually
```bash
## Debian as an example.
apt update && apt install -y apt-transport-https lsb-release wget curl software-properties-common gnupg g++-multilib git make
bash <(curl -fsSL https://apt.llvm.org/llvm.sh) 18

echo 'export PATH=/usr/lib/llvm-18/bin:$PATH' >> /etc/profile
echo 'export CC=clang' >> /etc/profile
echo 'export CXX=clang++' >> /etc/profile
echo 'export XMAKE_ROOT=y' >> /etc/profile
source /etc/profile

wget https://xmake.io/shget.text -O - | bash
source ~/.xmake/profile

mkdir temp && cd temp
git clone --depth=1 -b 1.12-dev --recurse-submodules https://github.com/alliedmodders/sourcemod sourcemod
git clone --depth=1 -b 1.11-dev https://github.com/alliedmodders/metamod-source metamod
git clone --depth=1 -b l4d2 https://github.com/alliedmodders/hl2sdk hl2sdk-l4d2
git clone --depth=1 https://github.com/fdxx/modeloader

cd modeloader
xmake f -c --SMPATH=../sourcemod --MMPATH=../metamod --HL2SDKPATH=../hl2sdk-l4d2
xmake -rv modeloader
```

Check the `release` folder, then rename `modeloader.cfg.example` to `modeloader.cfg`, make your awesome mode.

## Other
Design reference: [Confogl matchmode](https://github.com/SirPlease/L4D2-Competitive-Rework)
