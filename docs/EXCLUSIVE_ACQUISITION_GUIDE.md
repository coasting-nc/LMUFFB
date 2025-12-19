# Exclusive Device Acquisition - User Guide

## What is Exclusive Device Acquisition?

When LMUFFB connects to your steering wheel, it can acquire the device in two modes:

### 🟢 Exclusive Mode (Recommended)
- **What it means**: LMUFFB has exclusive control over Force Feedback
- **Benefits**:
  - Automatically prevents the game from sending FFB
  - Eliminates "Double FFB" conflicts
  - No need to manually disable in-game FFB
- **Game compatibility**: The game can still read your steering inputs perfectly
- **Display**: Green text "Mode: EXCLUSIVE (Game FFB Blocked)"

### 🟡 Shared Mode (Requires Manual Setup)
- **What it means**: LMUFFB is sharing the device with other applications
- **When this happens**: 
  - The game already grabbed exclusive access before LMUFFB started
  - Another FFB application is running
- **Required action**: You MUST disable in-game FFB or set it to 0% strength
- **Risk**: If you don't disable game FFB, two force signals will fight each other
- **Display**: Yellow text "Mode: SHARED (Potential Conflict)"

## How It Works

LMUFFB automatically tries to get exclusive access when you select a device:

1. **First Attempt**: Try to acquire device in Exclusive mode
2. **Success**: Device locked, game FFB automatically blocked ✅
3. **Failure**: Fall back to Shared mode, manual setup required ⚠️

## Troubleshooting

### I'm seeing "SHARED" mode but want "EXCLUSIVE"

**Solution**: Start LMUFFB before launching the game

1. Close the game completely
2. Close LMUFFB
3. Start LMUFFB first
4. Select your FFB device in LMUFFB
5. Verify you see green "EXCLUSIVE" mode
6. Now launch the game

### I'm in "SHARED" mode and can't change it

**Workaround**: Disable in-game FFB

1. In the game settings, find Force Feedback options
2. Set FFB strength to 0% or select "None"
3. This prevents the double FFB conflict
4. LMUFFB will still work normally

### The game FFB is fighting with LMUFFB

**Symptoms**:
- Wheel feels strange or oscillates
- Forces feel too strong or conflicting
- Wheel behavior is unpredictable

**Fix**:
- Check the acquisition mode in LMUFFB
- If SHARED: Disable in-game FFB completely
- If EXCLUSIVE: This shouldn't happen - report as a bug

## Best Practices

1. ✅ **Start LMUFFB first** before launching the game
2. ✅ **Check the mode indicator** after selecting your device
3. ✅ **If EXCLUSIVE**: You're good to go, no further action needed
4. ✅ **If SHARED**: Disable in-game FFB to avoid conflicts
5. ✅ **Use the tooltip**: Hover over the mode text for detailed info

## Technical Details

### Why does this matter?

DirectInput allows only ONE application to have exclusive FFB access. When LMUFFB gets exclusive access:
- The game can still read your steering angle, throttle, brake (inputs work normally)
- The game CANNOT send FFB commands (its FFB is automatically muted)
- Only LMUFFB controls the wheel forces

This is the ideal setup because:
- No manual configuration needed
- No risk of double FFB
- Cleaner, more predictable force feedback

### What if I want both apps to send FFB?

You don't. Trust us. Double FFB creates:
- Conflicting forces that fight each other
- Unpredictable wheel behavior  
- Loss of detail and feel
- Potential wheel oscillation

Always use EXCLUSIVE mode when possible, or disable one FFB source completely.

## FAQ

**Q: Will the game still work if LMUFFB has exclusive access?**  
A: Yes! The game can still read all your inputs (steering, pedals, buttons). Only the FFB output is blocked.

**Q: Can I switch from SHARED to EXCLUSIVE without restarting?**  
A: No. You need to close the game, unbind the device in LMUFFB, rebind it, then restart the game.

**Q: Does this affect other DirectInput devices?**  
A: No. This only affects the specific FFB device you select in LMUFFB.

**Q: What if I have multiple FFB devices?**  
A: Each device is acquired independently. LMUFFB will try exclusive mode for the device you select.

## Version Information

This feature was implemented in version 0.4.21 (2025-12-19)

For technical implementation details, see: `docs/dev_docs/implementation_summary_exclusive_acquisition.md`
