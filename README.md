# H2M-MOD: Downloader/Updater

This is a standalone console application based off the `Updater` from the [BOIII Client](https://github.com/CBServers/boiii) originally developed by [momo5502](https://github.com/momo5502) and [X Labs](https://xlabs.dev/).\
Thanks to all the original contributors.

## About
The purpose of this downloader/updater is to download safe H2M-Mod files with the option to also download Campaign and AW Maps. 

The `h2m-mod.exe` provided has the hash `9dcfda29748e29e806119cb17847bb3617c188b402ed743bd16e770401f9e127`. A known safe hash for H2M-Mod.

![image](https://github.com/user-attachments/assets/b52d34ac-358f-4056-acd2-57f737c27f73)\
Source: HorizonMW

## Download
- You can download the H2M-Mod Updater from [Releases](https://github.com/CBServers/h2m-updater/releases)
- Place `h2m-updater.exe` in the root of your Modern Warefare Remastered folder (from Steam or Torrent)
- Run `h2m-updater.exe` and follow the on-screen prompts

## Features
- Download safe H2M-Mod files
- Optionally download Campaign Maps and Advanced Warfare Maps
- Store Campaign and AW Map preferences in `h2m-update-config.json`
- Auto updater
- Optionally launch H2M-Mod after download/update

## Compile from source

- Clone the Git repo. Do NOT download it as ZIP, that won't work.
- Update the submodules and run `premake5 vs2022` or simply use the delivered `generate.bat`.
- Build via solution file in `build\h2m-updater.sln`.

## Disclaimer

This software has been created purely for the purposes of
academic research. It is not intended to be used to attack
other systems. Project maintainers are not responsible or
liable for misuse of the software. Use responsibly.
