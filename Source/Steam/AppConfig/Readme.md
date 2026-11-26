# Game specific workaround app config options
- Filename should be in the format of `Steam_<AppId>_<ExecutableName>.json`
- File format is FEX's regular application configuration file
- There should be a comment block living outside of the `Config` json section explaining the need.
- Ideally there would also be an entry on the FEX wiki explaining the issue as well.

# What is this useful for?
Working around game specific quirks necessary for some games to function.
- Example, A game requires x87 full precision but FEX disables it by default for Steam configurations. That should live in this directory.
