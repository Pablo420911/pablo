// RoninCraftBot – A lightweight C++ Discord to-do & planning bot
// Run: DISCORD_TOKEN=<your-token> ./RoninCraftBot
//
// Slash commands:
//   /todo add <task>       – Add a task
//   /todo list             – View your task list
//   /todo done <id>        – Mark a task complete
//   /todo remove <id>      – Delete a task
//   /todo clear            – Remove all completed tasks
//   /remind <minutes> <message> – Set a timed reminder (DM after N minutes)
//   /myreminders           – List your pending reminders

#include <dpp/dpp.h>
#include <nlohmann/json.hpp>

#include <atomic>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <chrono>

#include "todo_manager.h"
#include "reminder_manager.h"

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static std::string dataDir() {
    // Prefer data/ next to the binary; fall back to ./data
    std::string d = fs::path(fs::current_path()) / "data";
    fs::create_directories(d);
    return d;
}

static dpp::embed makeTodoEmbed(const std::vector<TodoItem>& items,
                                 const std::string&           username) {
    dpp::embed e;
    e.set_title("📋 " + username + "'s To-Do List");
    e.set_color(0x5865F2);   // Discord blurple

    if (items.empty()) {
        e.set_description("*Your list is empty — use `/todo add` to get started!*");
        return e;
    }

    std::ostringstream pending, done;
    for (auto& item : items) {
        auto& stream = item.done ? done : pending;
        stream << (item.done ? "~~" : "")
               << "**[" << item.id << "]** " << item.task
               << (item.done ? "~~" : "")
               << "\n";
    }

    if (!pending.str().empty())
        e.add_field("⏳ Pending", pending.str(), false);
    if (!done.str().empty())
        e.add_field("✅ Completed", done.str(), false);

    return e;
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main() {
    const char* tokenEnv = std::getenv("DISCORD_TOKEN");
    if (!tokenEnv || std::string(tokenEnv).empty()) {
        std::cerr << "[RoninCraftBot] ERROR: DISCORD_TOKEN environment variable is not set.\n";
        return 1;
    }
    const std::string token(tokenEnv);
    const std::string data = dataDir();

    TodoManager     todos(data);
    ReminderManager reminders(data);

    // Atomic flag for graceful reminder thread shutdown
    std::atomic<bool> running{true};

    dpp::cluster bot(token, dpp::i_default_intents | dpp::i_guild_presences);

    // -----------------------------------------------------------------------
    // Logging
    // -----------------------------------------------------------------------
    bot.on_log([](const dpp::log_t& ev) {
        if (ev.severity >= dpp::ll_warning) {
            std::cout << "[" << dpp::utility::loglevel(ev.severity) << "] "
                      << ev.message << "\n";
        }
    });

    // -----------------------------------------------------------------------
    // Register slash commands on ready
    // -----------------------------------------------------------------------
    bot.on_ready([&bot](const dpp::ready_t&) {
        std::cout << "[RoninCraftBot] Logged in as " << bot.me.username << "\n";

        if (dpp::run_once<struct register_bot_commands>()) {
            // /todo
            dpp::slashcommand todo_cmd("todo", "Manage your to-do list", bot.me.id);
            todo_cmd.add_option(
                dpp::command_option(dpp::co_sub_command, "add", "Add a new task")
                    .add_option(dpp::command_option(dpp::co_string, "task",
                                                    "What do you need to do?", true)));
            todo_cmd.add_option(
                dpp::command_option(dpp::co_sub_command, "list", "View your task list"));
            todo_cmd.add_option(
                dpp::command_option(dpp::co_sub_command, "done", "Mark a task as done")
                    .add_option(dpp::command_option(dpp::co_integer, "id",
                                                    "Task ID to mark complete", true)));
            todo_cmd.add_option(
                dpp::command_option(dpp::co_sub_command, "remove", "Delete a task")
                    .add_option(dpp::command_option(dpp::co_integer, "id",
                                                    "Task ID to remove", true)));
            todo_cmd.add_option(
                dpp::command_option(dpp::co_sub_command, "clear",
                                    "Remove all completed tasks"));

            // /remind
            dpp::slashcommand remind_cmd("remind",
                                         "Set a reminder that DMs you after N minutes",
                                         bot.me.id);
            remind_cmd.add_option(
                dpp::command_option(dpp::co_integer, "minutes",
                                    "Minutes from now to remind you", true));
            remind_cmd.add_option(
                dpp::command_option(dpp::co_string, "message",
                                    "What should I remind you about?", true));

            // /myreminders
            dpp::slashcommand myrem_cmd("myreminders",
                                        "List your pending reminders", bot.me.id);

            bot.global_bulk_command_create({todo_cmd, remind_cmd, myrem_cmd});
        }
    });

    // -----------------------------------------------------------------------
    // Slash command handler
    // -----------------------------------------------------------------------
    bot.on_slashcommand([&bot, &todos, &reminders](const dpp::slashcommand_t& ev) {
        const std::string uid  = std::to_string(ev.command.usr.id);
        const std::string name = ev.command.usr.username;
        const std::string cid  = std::to_string(ev.command.channel_id);
        const std::string cmd  = ev.command.get_command_name();

        // ------------------------------------------------------------------ /todo
        if (cmd == "todo") {
            const std::string sub = ev.command.get_command_interaction().options[0].name;

            if (sub == "add") {
                std::string task = std::get<std::string>(
                    ev.get_parameter("task"));
                int id = todos.addTodo(uid, task);
                ev.reply(dpp::message()
                    .set_flags(dpp::m_ephemeral)
                    .add_embed(dpp::embed()
                        .set_color(0x57F287)
                        .set_description("✅ Task **[" + std::to_string(id) +
                                         "]** added: *" + task + "*")));

            } else if (sub == "list") {
                auto items = todos.getTodos(uid);
                ev.reply(dpp::message()
                    .set_flags(dpp::m_ephemeral)
                    .add_embed(makeTodoEmbed(items, name)));

            } else if (sub == "done") {
                int id = static_cast<int>(std::get<int64_t>(ev.get_parameter("id")));
                if (todos.markDone(uid, id)) {
                    ev.reply(dpp::message()
                        .set_flags(dpp::m_ephemeral)
                        .add_embed(dpp::embed()
                            .set_color(0x57F287)
                            .set_description("✅ Task **[" + std::to_string(id) +
                                             "]** marked as done!")));
                } else {
                    ev.reply(dpp::message()
                        .set_flags(dpp::m_ephemeral)
                        .add_embed(dpp::embed()
                            .set_color(0xED4245)
                            .set_description("❌ Task **[" + std::to_string(id) +
                                             "]** not found.")));
                }

            } else if (sub == "remove") {
                int id = static_cast<int>(std::get<int64_t>(ev.get_parameter("id")));
                if (todos.removeTodo(uid, id)) {
                    ev.reply(dpp::message()
                        .set_flags(dpp::m_ephemeral)
                        .add_embed(dpp::embed()
                            .set_color(0xFEE75C)
                            .set_description("🗑️ Task **[" + std::to_string(id) +
                                             "]** removed.")));
                } else {
                    ev.reply(dpp::message()
                        .set_flags(dpp::m_ephemeral)
                        .add_embed(dpp::embed()
                            .set_color(0xED4245)
                            .set_description("❌ Task **[" + std::to_string(id) +
                                             "]** not found.")));
                }

            } else if (sub == "clear") {
                todos.clearDone(uid);
                ev.reply(dpp::message()
                    .set_flags(dpp::m_ephemeral)
                    .add_embed(dpp::embed()
                        .set_color(0x57F287)
                        .set_description("🧹 All completed tasks cleared!")));
            }

        // ---------------------------------------------------------------- /remind
        } else if (cmd == "remind") {
            int         mins = static_cast<int>(std::get<int64_t>(ev.get_parameter("minutes")));
            std::string msg  = std::get<std::string>(ev.get_parameter("message"));

            if (mins <= 0 || mins > 10080) {   // 1 week max
                ev.reply(dpp::message()
                    .set_flags(dpp::m_ephemeral)
                    .add_embed(dpp::embed()
                        .set_color(0xED4245)
                        .set_description("❌ Please choose between 1 and 10080 minutes (1 week).")));
                return;
            }

            int id = reminders.addReminder(uid, cid, msg, mins);
            ev.reply(dpp::message()
                .set_flags(dpp::m_ephemeral)
                .add_embed(dpp::embed()
                    .set_color(0x5865F2)
                    .set_title("⏰ Reminder set!")
                    .set_description("I'll DM you in **" + std::to_string(mins) +
                                     " minute" + (mins == 1 ? "" : "s") +
                                     "** about:\n> " + msg +
                                     "\n\n*(Reminder ID: " + std::to_string(id) + ")*")));

        // --------------------------------------------------------- /myreminders
        } else if (cmd == "myreminders") {
            auto rems = reminders.getReminders(uid);
            dpp::embed e;
            e.set_title("⏰ Your Pending Reminders");
            e.set_color(0x5865F2);
            if (rems.empty()) {
                e.set_description("*You have no pending reminders.*");
            } else {
                std::ostringstream ss;
                for (auto& r : rems) {
                    auto now  = std::time(nullptr);
                    long diff = static_cast<long>(r.remind_at - now);
                    ss << "**[" << r.id << "]** " << r.message << " — ";
                    if (diff <= 0) {
                        ss << "firing soon\n";
                    } else {
                        long hrs  = diff / 3600;
                        long mins = (diff % 3600) / 60;
                        long secs = diff % 60;
                        if (hrs > 0)  ss << hrs  << "h ";
                        if (mins > 0) ss << mins << "m ";
                        ss << secs << "s\n";
                    }
                }
                e.set_description(ss.str());
            }
            ev.reply(dpp::message().set_flags(dpp::m_ephemeral).add_embed(e));
        }
    });

    // -----------------------------------------------------------------------
    // Greet users who come online and show pending tasks
    // -----------------------------------------------------------------------
    bot.on_presence_update([&bot, &todos](const dpp::presence_update_t& ev) {
        // Only trigger when status changes to "online"
        if (ev.rich_presence.status() != dpp::ps_online) return;

        const std::string uid  = std::to_string(ev.rich_presence.user_id);
        auto items = todos.getTodos(uid);

        // Count pending tasks
        int pending = 0;
        for (auto& i : items) if (!i.done) ++pending;
        if (pending == 0) return;

        // Send a DM
        bot.create_dm_channel(
            ev.rich_presence.user_id,
            [&bot, uid, pending](const dpp::confirmation_callback_t& cc) {
                if (cc.is_error()) return;
                auto ch = std::get<dpp::channel>(cc.value);
                dpp::message dm;
                dm.channel_id = ch.id;
                dm.add_embed(
                    dpp::embed()
                        .set_title("👋 Welcome back!")
                        .set_color(0x5865F2)
                        .set_description(
                            "You have **" + std::to_string(pending) +
                            " pending task" + (pending == 1 ? "" : "s") +
                            "** on your to-do list.\n"
                            "Use `/todo list` in any server to review them.")
                );
                bot.message_create(dm);
            });
    });

    // -----------------------------------------------------------------------
    // Background thread: check reminders every 30 seconds
    // -----------------------------------------------------------------------
    std::thread reminderThread([&bot, &reminders, &running]() {
        while (running.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(30));
            if (!running.load()) break;
            reminders.tick([&bot](const Reminder& r) {
                // Create DM channel then send the reminder message
                dpp::snowflake uid(std::stoull(r.user_id));
                bot.create_dm_channel(
                    uid,
                    [&bot, r](const dpp::confirmation_callback_t& cc) {
                        if (cc.is_error()) return;
                        auto ch = std::get<dpp::channel>(cc.value);
                        dpp::message dm;
                        dm.channel_id = ch.id;
                        dm.add_embed(
                            dpp::embed()
                                .set_title("⏰ Reminder from RoninCraftBot")
                                .set_color(0xFEE75C)
                                .set_description(r.message)
                        );
                        bot.message_create(dm);
                    });
            });
        }
    });

    // -----------------------------------------------------------------------
    // Start the bot (blocks until killed)
    // -----------------------------------------------------------------------
    std::cout << "[RoninCraftBot] Starting — press Ctrl+C to stop.\n";
    bot.start(dpp::st_wait);

    // Signal reminder thread to exit and join cleanly
    running.store(false);
    reminderThread.join();
    return 0;
}
