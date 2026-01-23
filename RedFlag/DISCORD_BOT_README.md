# RedFlag Discord Bot 🤖

Discord bot extension for RedFlag that allows you to run malware scans directly from Discord.

## Features

- **Scan Commands**: Run RedFlag scans via Discord commands
- **File Upload Support**: Attach files/projects to scan them
- **Rich Embeds**: Beautiful formatted results with color-coded severity
- **JSON Export**: Export full scan results as JSON files
- **Queue Management**: Handles multiple scan requests

## Setup

### 1. Create a Discord Bot

1. Go to [Discord Developer Portal](https://discord.com/developers/applications)
2. Click "New Application" and give it a name
3. Go to the "Bot" section
4. Click "Add Bot" and confirm
5. Under "Token", click "Copy" to get your bot token
6. Enable "Message Content Intent" under "Privileged Gateway Intents"

### 2. Invite Bot to Server

1. Go to "OAuth2" → "URL Generator"
2. Select scopes: `bot` and `applications.commands`
3. Select bot permissions: `Send Messages`, `Read Message History`, `Attach Files`
4. Copy the generated URL and open it in your browser
5. Select your server and authorize

### 3. Install Dependencies

```bash
pip install discord.py>=2.0.0
```

Or install all RedFlag dependencies:

```bash
pip install -r requirements.txt
```

### 4. Run the Bot

#### Option A: Command Line

```bash
python discord_bot.py --token YOUR_BOT_TOKEN
```

#### Option B: Environment Variable

```bash
export DISCORD_BOT_TOKEN=YOUR_BOT_TOKEN
python discord_bot.py
```

#### Option C: Custom Prefix

```bash
python discord_bot.py --token YOUR_BOT_TOKEN --prefix "rf!"
```

## Commands

### `!redflag <path>` or `!scan <path>`

Scan a project or file for malicious indicators.

**Examples:**
```
!redflag /path/to/project
!scan C:\Users\Project
```

**With File Attachments:**
- Attach files to your message and use `!redflag` without a path
- The bot will download and scan the attached files

### `!redflag-json <path>`

Scan and export results as a JSON file.

**Example:**
```
!redflag-json /path/to/project
```

### `!redflag-help`

Show help message with all available commands.

### `!redflag-version`

Show RedFlag version information.

## Usage Examples

### Scanning a Local Project

```
User: !redflag /home/user/suspicious_project
Bot: [Sends embed with scan results]
```

### Scanning Attached Files

```
User: [Attaches malware.cpp] !redflag
Bot: [Downloads file, scans it, sends results]
```

### Exporting JSON Results

```
User: !redflag-json /path/to/project
Bot: [Sends JSON file attachment]
```

## Bot Permissions

Required permissions:
- **Send Messages**: To send scan results
- **Read Message History**: To read commands
- **Attach Files**: To send JSON exports
- **Embed Links**: To send formatted embeds

## Security Considerations

⚠️ **Important Security Notes:**

1. **Bot Token Security**: Never commit your bot token to version control
2. **File Access**: The bot can access any file the host system can access
3. **Rate Limiting**: Discord has rate limits; the bot handles them automatically
4. **Temporary Files**: Attached files are stored temporarily and cleaned up after scanning

## Troubleshooting

### Bot Doesn't Respond

- Check that the bot is online (green status in Discord)
- Verify the bot has "Message Content Intent" enabled
- Ensure the bot has permission to read messages in the channel

### "Command Not Found"

- Check your command prefix (default is `!`)
- Make sure you're using the correct command name
- Try `!redflag-help` to see available commands

### Import Errors

If you get `ImportError: discord.py is required`:

```bash
pip install discord.py
```

### Token Errors

If you get "Invalid Discord bot token":
- Verify you copied the full token
- Make sure you're using the bot token, not the client secret
- Check that the token hasn't been regenerated

## Advanced Configuration

### Running as a Service

On Linux, you can run the bot as a systemd service:

```ini
[Unit]
Description=RedFlag Discord Bot
After=network.target

[Service]
Type=simple
User=your-user
WorkingDirectory=/path/to/RedFlag
Environment="DISCORD_BOT_TOKEN=your-token"
ExecStart=/usr/bin/python3 /path/to/RedFlag/discord_bot.py
Restart=always

[Install]
WantedBy=multi-user.target
```

### Docker

```dockerfile
FROM python:3.9-slim

WORKDIR /app
COPY . .
RUN pip install -r requirements.txt

ENV DISCORD_BOT_TOKEN=""
CMD ["python", "discord_bot.py"]
```

## Contributing

Contributions welcome! Please open an issue or submit a pull request.

## License

Same license as RedFlag main project.
