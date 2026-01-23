"""
Discord Bot Extension for RedFlag
Allows running scans and viewing results via Discord commands
"""
import os
import sys
import asyncio
import tempfile
import shutil
from typing import Optional
from pathlib import Path

try:
    import discord
    from discord.ext import commands
    from discord import app_commands
    DISCORD_AVAILABLE = True
except ImportError:
    DISCORD_AVAILABLE = False
    discord = None
    commands = None
    app_commands = None

from .core.engine import RedFlagScanner
from .core.utils import UI
from .core.config import BANNER


class RedFlagBot(commands.Bot):
    """
    Discord bot for RedFlag malware analysis
    """
    
    def __init__(self, command_prefix='!', intents=None):
        if not DISCORD_AVAILABLE:
            raise ImportError("discord.py is required. Install with: pip install discord.py")
        
        if intents is None:
            intents = discord.Intents.default()
            intents.message_content = True
        
        super().__init__(command_prefix=command_prefix, intents=intents)
        self.scan_queue = []
        self.temp_dirs = []  # Track temp dirs for cleanup
        
    async def setup_hook(self):
        """Called when bot is starting up"""
        await self.add_cog(RedFlagCog(self))
        # Sync slash commands
        try:
            synced = await self.tree.sync()
            print(f'Synced {len(synced)} slash command(s)')
        except Exception as e:
            print(f'Failed to sync slash commands: {e}')
    
    async def on_ready(self):
        """Called when bot is ready"""
        print(f'{self.user} has connected to Discord!')
        print(f'Bot is in {len(self.guilds)} guild(s)')
        print(f'Prefix commands: {self.command_prefix}')
        print(f'Slash commands: /redflag, /redflag-json, /redflag-help, /redflag-version')
    
    async def close(self):
        """Cleanup on bot shutdown"""
        # Clean up temporary directories
        for temp_dir in self.temp_dirs:
            try:
                if os.path.exists(temp_dir):
                    shutil.rmtree(temp_dir)
            except Exception:
                pass
        await super().close()


class RedFlagCog(commands.Cog):
    """Commands for RedFlag scanning"""
    
    def __init__(self, bot: RedFlagBot):
        self.bot = bot
    
    # Slash command for scanning
    @app_commands.command(name='redflag', description='Scan a project or file for malicious indicators')
    @app_commands.describe(path='Path to project or file to scan')
    async def scan_slash(self, interaction: discord.Interaction, path: str):
        """Slash command version of scan"""
        await interaction.response.defer()
        
        # Scan local path
        target_path = path.strip().strip('"').strip("'")
        
        if not os.path.exists(target_path):
            await interaction.followup.send(f"❌ Path not found: `{target_path}`")
            return
        
        await self._scan_path_slash(interaction, target_path)
    
    # Prefix command for scanning
    @commands.command(name='redflag', aliases=['rf', 'scan'])
    async def scan_command(self, ctx: commands.Context, *, target: Optional[str] = None):
        """
        Scan a project for malicious indicators
        
        Usage:
        !redflag <path> - Scan a local path (bot must have access)
        !redflag - Scan attached files/project
        """
        await ctx.typing()
        
        # Check for file attachments
        if ctx.message.attachments:
            if target:
                await ctx.send("❌ Please provide either a path OR attach files, not both.")
                return
            
            # Download and scan attached files
            await self._scan_attachments(ctx, ctx.message.attachments)
            return
        
        # Scan local path
        if not target:
            await ctx.send("❌ Please provide a path to scan or attach files.\n"
                          "Usage: `!redflag <path>` or attach files to your message.")
            return
        
        target_path = target.strip().strip('"').strip("'")
        
        if not os.path.exists(target_path):
            await ctx.send(f"❌ Path not found: `{target_path}`")
            return
        
        await self._scan_path(ctx, target_path)
    
    async def _scan_attachments(self, ctx: commands.Context, attachments):
        """Download and scan attached files"""
        temp_dir = tempfile.mkdtemp(prefix='redflag_discord_')
        self.bot.temp_dirs.append(temp_dir)
        
        try:
            # Download all attachments
            file_paths = []
            for attachment in attachments:
                file_path = os.path.join(temp_dir, attachment.filename)
                await attachment.save(file_path)
                file_paths.append(file_path)
            
            # If single file, scan it directly; otherwise scan the directory
            if len(file_paths) == 1 and os.path.isfile(file_paths[0]):
                target = file_paths[0]
            else:
                target = temp_dir
            
            await self._scan_path(ctx, target, is_temp=True)
            
        except Exception as e:
            await ctx.send(f"❌ Error processing attachments: {str(e)}")
    
    async def _scan_path(self, ctx: commands.Context, target_path: str, is_temp: bool = False):
        """Run RedFlag scan on a path"""
        try:
            # Create scanner
            scanner = RedFlagScanner(target_path, show_definitions=False)
            
            # Capture output (we'll format it for Discord)
            findings_summary = []
            
            # Run scan in executor to avoid blocking
            loop = asyncio.get_event_loop()
            await loop.run_in_executor(None, scanner.run)
            
            # Build summary
            total_score = sum(f.score for f in scanner.findings if f.severity != "INFO")
            severities = [f.severity for f in scanner.findings]
            max_severity = "CLEAN"
            if "CRITICAL" in severities:
                max_severity = "CRITICAL"
            elif "HIGH" in severities:
                max_severity = "HIGH"
            elif "MEDIUM" in severities:
                max_severity = "MEDIUM"
            elif "LOW" in severities:
                max_severity = "LOW"
            elif "INFO" in severities:
                max_severity = "INFO"
            
            # Create embed
            color_map = {
                "CRITICAL": discord.Color.red(),
                "HIGH": discord.Color.orange(),
                "MEDIUM": discord.Color.gold(),
                "LOW": discord.Color.blue(),
                "INFO": discord.Color.green(),
                "CLEAN": discord.Color.green()
            }
            
            embed = discord.Embed(
                title="🔴 RedFlag Scan Results",
                description=f"**Target:** `{os.path.basename(target_path)}`\n"
                           f"**Project Type:** {scanner.project_type}",
                color=color_map.get(max_severity, discord.Color.default())
            )
            
            embed.add_field(
                name="Risk Level",
                value=f"**{max_severity}**",
                inline=True
            )
            embed.add_field(
                name="Threat Score",
                value=str(total_score),
                inline=True
            )
            embed.add_field(
                name="Total Findings",
                value=str(len(scanner.findings)),
                inline=True
            )
            
            # Count by severity
            severity_counts = {
                "CRITICAL": len([f for f in scanner.findings if f.severity == "CRITICAL"]),
                "HIGH": len([f for f in scanner.findings if f.severity == "HIGH"]),
                "MEDIUM": len([f for f in scanner.findings if f.severity == "MEDIUM"]),
                "LOW": len([f for f in scanner.findings if f.severity == "LOW"]),
                "INFO": len([f for f in scanner.findings if f.severity == "INFO"])
            }
            
            counts_text = "\n".join([
                f"{sev}: {count}" 
                for sev, count in severity_counts.items() 
                if count > 0
            ])
            if counts_text:
                embed.add_field(
                    name="Findings by Severity",
                    value=f"```{counts_text}```",
                    inline=False
                )
            
            # Top findings (limit to 10 for Discord embed)
            if scanner.findings:
                scanner.findings.sort(key=lambda x: x.score, reverse=True)
                top_findings = scanner.findings[:10]
                
                findings_text = []
                for i, f in enumerate(top_findings, 1):
                    file_display = f.file
                    if len(file_display) > 40:
                        file_display = "..." + file_display[-37:]
                    findings_text.append(
                        f"{i}. **{f.severity}** - {f.description}\n"
                        f"   `{file_display}:{f.line}`"
                    )
                
                if findings_text:
                    # Discord embed field value limit is 1024 chars
                    findings_str = "\n".join(findings_text)
                    if len(findings_str) > 1024:
                        findings_str = findings_str[:1021] + "..."
                    
                    embed.add_field(
                        name="Top Findings",
                        value=findings_str,
                        inline=False
                    )
            
            # Footer
            embed.set_footer(text="RedFlag v1.5.0 | Use !redflag-help for more commands")
            
            await ctx.send(embed=embed)
            
            # If there are many findings, offer JSON export
            if len(scanner.findings) > 10:
                await ctx.send(
                    "💡 **Tip:** Use `/redflag-json` or `!redflag-json` to export full results as JSON file."
                )
            
        except Exception as e:
            await ctx.send(f"❌ **Error during scan:**\n```{str(e)}```")
            import traceback
            print(f"RedFlag Discord Bot Error: {traceback.format_exc()}")
    
    @commands.command(name='redflag-json', aliases=['rf-json'])
    async def export_json(self, ctx: commands.Context, *, target: Optional[str] = None):
        """
        Export scan results as JSON file
        
        Usage: !redflag-json <path>
        """
        await ctx.typing()
        
        if not target:
            await ctx.send("❌ Please provide a path to scan.\n"
                          "Usage: `!redflag-json <path>`")
            return
        
        target_path = target.strip().strip('"').strip("'")
        
        if not os.path.exists(target_path):
            await ctx.send(f"❌ Path not found: `{target_path}`")
            return
        
        try:
            scanner = RedFlagScanner(target_path, show_definitions=False)
            
            loop = asyncio.get_event_loop()
            await loop.run_in_executor(None, scanner.run)
            
            # Export to temporary file
            temp_file = tempfile.NamedTemporaryFile(
                mode='w', 
                suffix='.json', 
                delete=False,
                prefix='redflag_results_'
            )
            temp_file_path = temp_file.name
            temp_file.close()
            
            from .cogs.verdict import export_to_json
            export_to_json(scanner, temp_file_path)
            
            # Send file
            await ctx.send(
                content=f"📄 **Scan Results JSON**\n"
                       f"**Target:** `{os.path.basename(target_path)}`\n"
                       f"**Findings:** {len(scanner.findings)}",
                file=discord.File(temp_file_path, filename='redflag_results.json')
            )
            
            # Cleanup
            try:
                os.unlink(temp_file_path)
            except Exception:
                pass
                
        except Exception as e:
            await ctx.send(f"❌ **Error:**\n```{str(e)}```")
    
    # Slash command for help
    @app_commands.command(name='redflag-help', description='Show RedFlag bot commands and usage')
    async def help_slash(self, interaction: discord.Interaction):
        """Slash command version of help"""
        await interaction.response.send_message(embed=self._create_help_embed())
    
    # Prefix command for help
    @commands.command(name='redflag-help', aliases=['rf-help', 'redflag-commands'])
    async def help_command(self, ctx: commands.Context):
        """Show RedFlag bot commands"""
        await ctx.send(embed=self._create_help_embed())
    
    @commands.command(name='redflag-version', aliases=['rf-version'])
    async def version_command(self, ctx: commands.Context):
        """Show RedFlag version"""
        from . import __version__
        await ctx.send(f"🔴 **RedFlag v{__version__}**\n"
                      f"Malware Analysis Tool for C++ Projects")


def run_bot(token: str, prefix: str = '!'):
    """
    Run the RedFlag Discord bot
    
    Args:
        token: Discord bot token
        prefix: Command prefix (default: !)
    """
    if not DISCORD_AVAILABLE:
        print("ERROR: discord.py is required!")
        print("Install with: pip install discord.py")
        sys.exit(1)
    
    intents = discord.Intents.default()
    intents.message_content = True
    
    bot = RedFlagBot(command_prefix=prefix, intents=intents)
    
    @bot.event
    async def on_command_error(ctx: commands.Context, error: commands.CommandError):
        """Handle command errors"""
        if isinstance(error, commands.CommandNotFound):
            return  # Ignore unknown commands
        elif isinstance(error, commands.MissingRequiredArgument):
            await ctx.send(f"❌ Missing required argument: `{error.param.name}`")
        else:
            await ctx.send(f"❌ **Error:** {str(error)}")
    
    try:
        bot.run(token)
    except discord.LoginFailure:
        print("ERROR: Invalid Discord bot token!")
        sys.exit(1)
    except KeyboardInterrupt:
        print("\nBot shutdown requested.")


if __name__ == "__main__":
    import argparse
    
    parser = argparse.ArgumentParser(description="RedFlag Discord Bot")
    parser.add_argument("--token", type=str, required=True, help="Discord bot token")
    parser.add_argument("--prefix", type=str, default="!", help="Command prefix (default: !)")
    args = parser.parse_args()
    
    run_bot(args.token, args.prefix)
