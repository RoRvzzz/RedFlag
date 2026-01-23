#!/usr/bin/env python3
"""
Discord Bot Launcher for RedFlag
Run this script to start the Discord bot
"""
import sys
import os

# Add current directory to path
script_dir = os.path.dirname(os.path.abspath(__file__))
if script_dir not in sys.path:
    sys.path.insert(0, script_dir)

from redflag.discord_bot import run_bot

if __name__ == "__main__":
    import argparse
    
    parser = argparse.ArgumentParser(description="RedFlag Discord Bot")
    parser.add_argument(
        "--token", 
        type=str, 
        help="Discord bot token (or set DISCORD_BOT_TOKEN env var)"
    )
    parser.add_argument(
        "--prefix", 
        type=str, 
        default="!", 
        help="Command prefix (default: !)"
    )
    args = parser.parse_args()
    
    token = args.token or os.getenv("DISCORD_BOT_TOKEN")
    
    if not token:
        print("ERROR: Discord bot token required!")
        print("Provide via --token argument or DISCORD_BOT_TOKEN environment variable")
        print("\nTo get a token:")
        print("1. Go to https://discord.com/developers/applications")
        print("2. Create a new application")
        print("3. Go to 'Bot' section and create a bot")
        print("4. Copy the token")
        sys.exit(1)
    
    print("Starting RedFlag Discord Bot...")
    print(f"Command prefix: {args.prefix}")
    print("Bot will be online shortly...")
    
    run_bot(token, args.prefix)
