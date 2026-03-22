# 📢 lmuFFB Update: The New Preset System Explained

Welcome to the new and improved preset system! We’ve completely overhauled how lmuFFB saves your settings to make it safer, cleaner, and much easier to share your favorite force feedback profiles with the community.

Here is everything you need to know about the changes.

### 1. Automatic Migration (Do I need to do anything?)
**No! The transition is 100% automatic.** 
The first time you run the new version of lmuFFB, it will automatically detect your old `config.ini` file, safely upgrade all your settings and custom presets to the new format, and create a backup of your old file named `config.ini.bak`. You won't lose a single setting.

### 2. What Changed? (INI is now TOML)
We have moved from `.ini` files to `.toml` files. TOML is just a standard text file (you can still open it in Notepad!), but it is much stricter and safer. It prevents bugs where a typo in your config file could crash the app or reset your settings.
* Your main settings file is now called **`config.toml`**.

### 3. Your Presets Now Have Their Own Folder!
In the past, all of your custom presets were stuffed at the very bottom of your main config file. This made them hard to find and hard to share. 

Now, every custom preset you create gets its own dedicated file inside a new **`user_presets`** folder located right next to the app.

**Your lmuFFB folder will now look like this:**
```text
lmuFFB/
 ├── lmuFFB.exe
 ├── config.toml             <-- Your main app settings
 └── user_presets/           <-- Your custom profiles live here!
      ├── My_Custom_Wheel.toml
      └── Drift_Setup.toml
```

### 4. Built-in vs. Custom Presets
To prevent accidental overwrites of the factory defaults, the presets that ship with the app (like the T300, Simagic, or Moza baselines) are now permanently baked into the app. 
* In the UI dropdown, these will show up with a **`[Default]`** prefix (e.g., `[Default] T300`).
* You cannot delete or overwrite `[Default]` presets. If you want to tweak one, simply make your changes and click **"Save New"** to create your own custom copy!

### 5. Sharing and Importing Presets
Sharing your FFB settings with friends is now incredibly easy:
* **To Share:** Just go into your `user_presets` folder, grab the `.toml` file you want to share, and drop it into Discord!
* **To Import:** You can either drop a downloaded `.toml` file directly into your `user_presets` folder, or use the **"Import Preset..."** button in the lmuFFB UI.

**"What if I download an old `.ini` preset from an old Discord post?"**
Don't worry! The "Import Preset" button in the UI is fully backwards-compatible. If you import an old `.ini` preset, lmuFFB will automatically upgrade its physics math to the latest version and save it into your `user_presets` folder as a shiny new `.toml` file.

---

### 💡 Quick FAQ

**Q: I opened `config.toml` in Notepad and it looks a bit different?**
A: Yes! TOML uses slightly different formatting (for example, `true`/`false` instead of `1`/`0`, and text has quotes around it like `"My Wheel"`). If you like manually editing your files, just follow the format you see in the file.

**Q: Can I delete `config.ini.bak`?**
A: Yes. Once you have run the new version and confirmed all your settings and presets loaded correctly, you can safely delete the `.bak` file.

**Q: I accidentally messed up my settings. How do I start fresh?**
A: Just select any of the `[Default]` presets from the dropdown menu to instantly restore the factory baseline.