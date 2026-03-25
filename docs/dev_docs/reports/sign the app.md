

## sign your app "unofficially"

To sign your app "unofficially" without paying for a commercial Code Signing Certificate, you can create a **Self-Signed Certificate**. 

### What this will and won't do:
* **What it WON'T do:** It will not instantly bypass the blue Windows SmartScreen ("Windows protected your PC") warning for your users. Because the certificate isn't issued by a trusted Certificate Authority (like DigiCert or Sectigo), Windows will still flag it as an "Unknown Publisher."
* **What it WILL do:** 
  1. It cryptographically seals your `.exe`. If an actual virus infects the file later, the signature breaks.
  2. It changes the structural layout of your `.exe` (adding a certificate table). This often **lowers the Machine Learning (ML) heuristic score** because the file no longer looks like a raw, unsigned, compiled-in-a-basement binary.
  3. **Reputation Building:** If you use the *exact same* self-signed certificate for every future release, Microsoft SmartScreen will eventually build a "reputation" for that certificate as your users download it and click "Run anyway." Over time, the warnings can subside.

Here is the step-by-step guide to generating a self-signed certificate and signing `lmuffb.exe` for free using built-in Windows tools.

---

### Step 1: Generate the Self-Signed Certificate
You will use PowerShell to generate a code-signing certificate on your machine.

1. Open **PowerShell** as Administrator.
2. Run the following command to create the certificate in your personal certificate store (change `"CN=lmuFFB Developer"` to whatever name you want):

```powershell
$cert = New-SelfSignedCertificate -Type CodeSigningCert -Subject "CN=lmuFFB Developer" -KeyExportPolicy Exportable -KeySpec Signature -HashAlgorithm sha256 -KeyLength 2048 -CertStoreLocation "Cert:\CurrentUser\My"
```

```powershell
# 1. Generate the certificate with explicit metadata
$cert = New-SelfSignedCertificate -Type CodeSigningCert `
    -Subject "CN=lmuFFB Developer, O=lmuFFB Project" `
    -FriendlyName "lmuFFB Code Signing" `
    -KeyExportPolicy Exportable `
    -KeySpec Signature `
    -HashAlgorithm sha256 `
    -KeyLength 2048 `
    -CertStoreLocation "Cert:\CurrentUser\My"

# To use this certificate in your build process (or on other machines), you need to export it as a `.pfx` file with a password.

# 2. Create a secure password
# Change `"YourPassword123"` to a password of your choice:
$pwd = ConvertTo-SecureString -String "YourSecurePassword123" -Force -AsPlainText

# 3. Export the certificate to a PFX file on your C: drive (or wherever you prefer)
Export-PfxCertificate -Cert $cert -FilePath "C:\lmuffb_cert.pfx" -Password $pwd

# You now have a file named `lmuffb_cert.pfx`. Keep this file safe; you will use it to sign all future releases of your app.

# 4. Convert directly to Base64 text  
[Convert]::ToBase64String([IO.File]::ReadAllBytes("C:\lmuffb_cert.pfx")) | Set-Content "C:\cert_base64.txt"
```

 

### Step 3: Sign your Executable
To sign the `.exe`, you need `signtool.exe`, which is included in the Windows SDK (if you are using Visual Studio, you already have this).

1. Open the **Developer Command Prompt for VS** (search for it in your Start menu). This ensures `signtool` is in your system PATH.
2. Navigate to the folder containing your compiled `lmuffb.exe` and your `lmuffb_cert.pfx`.
3. Run the following command to sign the executable:

```cmd
signtool sign /f "C:\lmuffb_cert.pfx" /p "YourPassword123" /tr http://timestamp.digicert.com /td sha256 /fd sha256 "lmuffb.exe"
```

#### Why the `/tr` (Timestamp) flag is critical:
The `/tr http://timestamp.digicert.com` part tells a public time-server to cryptographically stamp the exact time you signed the file. Self-signed certificates expire (usually after 1 year). If you timestamp the signature, Windows will consider the signature valid **forever**, because it proves the file was signed *before* the certificate expired.

### Step 4: Verify the Signature
Right-click your newly signed `lmuffb.exe` in Windows Explorer, select **Properties**, and you should now see a **Digital Signatures** tab. It will list "lmuFFB Developer". 

If you click "Details", it will say the certificate is not trusted (which is expected for self-signed), but the signature itself is valid.

---

### How to handle this with your users
When you release v0.7.223, you can add a small note to your release page:

> *"Note: This release is self-signed to help prevent false-positive antivirus deletions. When you run it for the first time, Windows SmartScreen may show a blue warning saying 'Unknown Publisher'. Click **More info** -> **Run anyway**."*

Combined with the code changes (removing `system()`, moving TOML to `.rc`, and stopping the `.ini` renaming), this self-signature should be enough to stop Windows Defender from outright deleting the file via its ML heuristic engine.

## use your self-signed certificate in a GitHub Actions 

To securely use your self-signed certificate in a GitHub Actions workflow, you must never commit the `.pfx` file directly to your repository. Instead, you will convert the binary `.pfx` file into a text string (Base64), store it securely in **GitHub Secrets**, and decode it on-the-fly during the build process.

Here is the step-by-step guide to setting this up safely.

### Step 1: Convert your `.pfx` file to a Base64 String
GitHub Secrets only accept text, so we need to convert your binary certificate into a Base64 text string.

Open **PowerShell** on your local machine and run this command (adjust the paths if your certificate is stored elsewhere):

```powershell[Convert]::ToBase64String([IO.File]::ReadAllBytes("C:\lmuffb_cert.pfx")) | Set-Content "C:\cert_base64.txt"
```
This will create a text file at `C:\cert_base64.txt`. Open this file in Notepad and copy all the text inside it (it will look like a massive block of random letters and numbers).

### Step 2: Add the Secrets to GitHub
Go to your repository on GitHub and navigate to:
**Settings** -> **Secrets and variables** -> **Actions** -> **New repository secret**

You need to create two secrets:
1. **Name:** `CERT_PFX_BASE64`
   **Secret:** *(Paste the entire contents of `cert_base64.txt` here)*
2. **Name:** `CERT_PASSWORD`
   **Secret:** *(Type the password you created for the `.pfx` file, e.g., `YourPassword123`)*

### Step 3: Update your GitHub Actions Workflow (`.yml`)
Open your release workflow file (usually located in `.github/workflows/release.yml`). You need to add a few steps right **after** your app compiles, but **before** it gets zipped or uploaded to the release.

Here is the exact YAML snippet to add:

```yaml
      # (Your existing build steps go here...)

      - name: Decode Certificate
        run: |
          $pfx_cert_byte = [System.Convert]::FromBase64String("${{ secrets.CERT_PFX_BASE64 }}")
          [IO.File]::WriteAllBytes("cert.pfx", $pfx_cert_byte)
        shell: pwsh

      # This action sets up the Windows Developer environment so 'signtool' is available in the PATH.
      # (You might already have this in your workflow if you are building with MSVC/CMake)
      - name: Setup MSVC environment
        uses: ilammy/msvc-dev-cmd@v1

      - name: Sign Executable
        run: |
          signtool sign /f cert.pfx /p "${{ secrets.CERT_PASSWORD }}" /tr http://timestamp.digicert.com /td sha256 /fd sha256 path\to\your\compiled\lmuffb.exe
        shell: pwsh

      - name: Cleanup Certificate
        run: Remove-Item cert.pfx
        shell: pwsh

      # (Your existing zip/upload steps go here...)
```

### Important Notes on the Workflow:
1. **Path to Executable:** Make sure to change `path\to\your\compiled\lmuffb.exe` in the `Sign Executable` step to the actual path where your `.exe` is generated during the GitHub Action run (e.g., `build\Release\lmuffb.exe`).
2. **Security (Cleanup):** The `Cleanup Certificate` step is crucial. It deletes the `cert.pfx` file from the runner immediately after signing. This ensures that if your next step zips up the whole directory, you don't accidentally publish your private certificate to the public GitHub release!
3. **Secret Masking:** GitHub automatically masks secrets in the action logs. If the password or Base64 string somehow gets printed to the console, GitHub will replace it with `***`, keeping your certificate safe.

Once this is merged, your GitHub Action will automatically decode the certificate, sign the `.exe`, timestamp it via DigiCert, and clean up after itself, resulting in a signed binary ready for your users.