/*
 * What: Implementation of Universal Chess Interface (UCI) parser and async worker.
 * Why:  Handles GUI handshakes, position setup, clock management, and search worker thread.
 * Layer: Layer 4 (interface) - depends on uci.h, core, movegen, and search.
 */

#include "uci.h"
#include <sstream>
#include <cctype>
#include "core/version.h"
#include "movegen/movegen.h"
#include "search/transposition_table.h"
#include "search/features.h"

namespace lookahead::interface {

UciHandler::UciHandler(std::istream& in, std::ostream& out)
    : in_(in), out_(out) {
    pos_.set_startpos();
}

UciHandler::~UciHandler() {
    stop_and_join();
}

void UciHandler::stop() {
    stop_and_join();
}

void UciHandler::stop_and_join() noexcept {
    stop_signal_.store(true, std::memory_order_release);
    if (search_thread_.joinable()) {
        search_thread_.join();
    }
    searching_.store(false, std::memory_order_release);
}

void UciHandler::loop() {
    std::string line;
    while (running_ && std::getline(in_, line)) {
        handle_command(line);
    }
    stop_and_join();
}

void UciHandler::handle_command(std::string_view line) {
    while (!line.empty() && std::isspace(static_cast<unsigned char>(line.front()))) {
        line.remove_prefix(1);
    }
    while (!line.empty() && std::isspace(static_cast<unsigned char>(line.back()))) {
        line.remove_suffix(1);
    }
    if (line.empty()) return;

    const size_t first_space = line.find(' ');
    const std::string_view cmd = (first_space != std::string_view::npos) ? line.substr(0, first_space) : line;
    const std::string_view rest = (first_space != std::string_view::npos) ? line.substr(first_space + 1) : "";

    if (cmd == "uci") {
        std::lock_guard<std::mutex> lock(out_mutex_);
        out_ << "id name " << core::ENGINE_NAME << " " << core::ENGINE_VERSION << "\n";
        out_ << "id author " << core::ENGINE_AUTHOR << "\n";
        out_ << "option name Hash type spin default 16 min 1 max 1024\n";
        out_ << "option name Clear Hash type button\n";
        out_ << "option name TranspositionTable type check default true\n";
        out_ << "option name QuiescenceSearch type check default true\n";
        out_ << "option name MoveOrdering type check default true\n";
        out_ << "option name NullMovePruning type check default true\n";
        out_ << "option name LateMoveReductions type check default true\n";
        out_ << "option name ReverseFutility type check default true\n";
        out_ << "option name AspirationWindows type check default true\n";
        out_ << "uciok\n";
        out_.flush();
    } else if (cmd == "isready") {
        std::lock_guard<std::mutex> lock(out_mutex_);
        out_ << "readyok\n";
        out_.flush();
    } else if (cmd == "stop") {
        stop_and_join();
    } else if (cmd == "ucinewgame") {
        stop_and_join();
        search::global_tt.clear();
        pos_.set_startpos();
    } else if (cmd == "position") {
        stop_and_join();
        handle_position(rest);
    } else if (cmd == "go") {
        stop_and_join();
        handle_go(rest);
    } else if (cmd == "setoption") {
        stop_and_join();
        handle_setoption(rest);
    } else if (cmd == "quit") {
        stop_and_join();
        running_ = false;
    }
}

void UciHandler::handle_position(std::string_view args) {
    std::istringstream iss{std::string(args)};
    std::string token;
    if (!(iss >> token)) return;

    if (token == "startpos") {
        pos_.set_startpos();
        iss >> token;
    } else if (token == "fen") {
        std::string fen;
        while (iss >> token && token != "moves") {
            if (!fen.empty()) fen += " ";
            fen += token;
        }
        if (!pos_.set_fen(fen)) {
            return;
        }
    }

    if (token == "moves") {
        std::string move_str;
        while (iss >> move_str) {
            movegen::MoveList legal_moves;
            movegen::generate_legal_moves(pos_, legal_moves);
            bool played = false;
            for (size_t i = 0; i < legal_moves.size(); ++i) {
                if (legal_moves[i].to_uci() == move_str) {
                    core::UndoInfo undo;
                    pos_.make_move(legal_moves[i], undo);
                    played = true;
                    break;
                }
            }
            if (!played) break;
        }
    }
}

void UciHandler::handle_go(std::string_view args) {
    std::istringstream iss{std::string(args)};
    std::string token;
    search::SearchLimits limits;

    while (iss >> token) {
        if (token == "depth") {
            iss >> limits.depth;
        } else if (token == "movetime") {
            iss >> limits.move_time_ms;
        } else if (token == "wtime") {
            iss >> limits.wtime_ms;
        } else if (token == "btime") {
            iss >> limits.btime_ms;
        } else if (token == "winc") {
            iss >> limits.winc_ms;
        } else if (token == "binc") {
            iss >> limits.binc_ms;
        } else if (token == "nodes") {
            iss >> limits.max_nodes;
        } else if (token == "movestogo") {
            iss >> limits.moves_to_go;
        } else if (token == "infinite") {
            limits.depth = 64;
        }
    }

    if (limits.depth == 0 && limits.move_time_ms == 0 && limits.wtime_ms == 0 && limits.btime_ms == 0) {
        limits.depth = 6;
    }

    stop_signal_.store(false, std::memory_order_release);
    searching_.store(true, std::memory_order_release);

    search_thread_ = std::thread([this, search_pos = pos_, limits]() mutable {
        const auto res = search::search(search_pos, limits, [this](const search::SearchInfo& info) {
            std::lock_guard<std::mutex> lock(out_mutex_);
            out_ << "info depth " << info.depth
                 << " score cp " << info.score
                 << " nodes " << info.nodes
                 << " nps " << static_cast<uint64_t>(info.nps)
                 << " time " << static_cast<int64_t>(info.elapsed_seconds * 1000.0);
            if (!info.pv_string.empty()) {
                out_ << " pv " << info.pv_string;
            }
            out_ << "\n";
            out_.flush();
        }, &stop_signal_);

        {
            std::lock_guard<std::mutex> lock(out_mutex_);
            out_ << "bestmove " << res.best_move.to_uci() << "\n";
            out_.flush();
        }
        searching_.store(false, std::memory_order_release);
    });
}

void UciHandler::handle_setoption(std::string_view args) {
    std::istringstream iss{std::string(args)};
    std::string token;
    std::string name;
    std::string value;

    if (!(iss >> token) || token != "name") return;

    while (iss >> token && token != "value") {
        if (!name.empty()) name += " ";
        name += token;
    }

    if (token == "value") {
        while (iss >> token) {
            if (!value.empty()) value += " ";
            value += token;
        }
    }

    if (name == "Hash" && !value.empty()) {
        try {
            const int mb = std::stoi(value);
            search::global_tt.resize(mb);
        } catch (...) {}
    } else if (name == "Clear Hash") {
        search::global_tt.clear();
    } else if (!value.empty()) {
        const bool enabled = (value == "true" || value == "1");
        if (name == "TranspositionTable") search::active_features.use_transposition_table = enabled;
        else if (name == "QuiescenceSearch") search::active_features.use_quiescence_search = enabled;
        else if (name == "MoveOrdering") search::active_features.use_move_ordering = enabled;
        else if (name == "NullMovePruning") search::active_features.use_null_move_pruning = enabled;
        else if (name == "LateMoveReductions") search::active_features.use_late_move_reductions = enabled;
        else if (name == "ReverseFutility") search::active_features.use_reverse_futility = enabled;
        else if (name == "AspirationWindows") search::active_features.use_aspiration_windows = enabled;
    }
}

}  // namespace lookahead::interface
