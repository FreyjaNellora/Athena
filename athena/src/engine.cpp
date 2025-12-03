#include "engine.h"
#include "utility.h"
#include "movegen.h"
#include "perft.h"
#include "search.h"   // for negamax, SCORE_INFINITY
#include "thread.h"   // for Thread

namespace athena
{

Engine::Engine() : pos()
{
    fromString(FEN_MODERN, pos);

    auto* uciCommand = app.add_subcommand("uci", "[UCI] Start UCI protocol and identify the engine")
        ->callback([this]() { handleUCI(); });

    auto* isreadyCommand = app.add_subcommand("isready", "Ensure engine is fully initialized before continuing")
        ->callback([this]() { handleIsReady(); });

    auto* setOptionCommand = app.add_subcommand("setoption", "Set an engine option in UCI format")
        ->callback([this]() { handleSetOption(); });

    setOptionCommand->allow_extras();

    auto* ucinewgameCommand = app.add_subcommand("ucinewgame", "Start a new game")
        ->callback([this]() { handleUCINewGame(); });

    auto* positionCommand = app.add_subcommand("position", "Position setup and display")
        ->callback([this]() { handlePosition(); });
    
    positionCommand->add_option("mode", position_mode, "Position setup mode")
        ->required()
        ->check(CLI::IsMember({"classic", "modern", "fen"}));
    
    positionCommand->allow_extras();
    
    auto* goCommand = app.add_subcommand("go", "Start a search")
        ->callback([this]() { handleGo(); });
    goCommand->allow_extras();  // allow "go depth 1" etc.


    auto* stopCommand = app.add_subcommand("stop", "Stop the current search")
        ->callback([this]() { handleStop(); });

    auto* exitCommand = app.add_subcommand("quit", "Quit the engine")
        ->callback([this]() { handleQuit(); });

    auto* perftCommand = app.add_subcommand("perft", "Run perft to given depth")
        ->callback([this]() { handlePerft(); });

    perftCommand->add_option("depth", perft_depth, "Depth to run perft")
        ->required()
        ->check(CLI::PositiveNumber);

    perftCommand->add_flag("-f,--full", perft_full, "Show full detailed report");
    perftCommand->add_flag("-s,--split", perft_split, "Show perft per move (split node counts)");
    perftCommand->add_flag("-c,--cumulative", perft_cumulative, "Show cumulative totals at each depth");

    auto* printCommand = app.add_subcommand("print", "Print current position")
        ->callback([this]() { handlePrint(); });

    printCommand->add_flag("-c,--config", print_config, "Show current engine configuration");
    printCommand->add_flag("-f,--fen", print_fen, "Print position in FEN format");
    printCommand->add_flag("-a,--ascii", print_ascii_pieces, "Print board as ASCII layout");

    auto* athenaFen4Command = app.add_subcommand("athena_fen4", "[GUI] Print a 14x14 dummy FEN4 board for testing")
        ->callback([this]() { handleAthenaFen4(); });
}

void Engine::launch()
{
    std::string line;
    while (true)
    {
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;

        std::transform(line.begin(), line.end(), line.begin(), ::tolower);
        std::vector<std::string> args = tokenize(line);
        args.insert(args.begin(), "athena");
        
        std::vector<const char*> argv;
        for (const auto& arg : args) argv.push_back(arg.c_str());
        int argc = static_cast<int>(argv.size());
        
        execute(argc, argv.data());
    }
}

void Engine::execute(int argc, const char* argv[])
{
    try
    {
        app.clear();

        // reset flags
        perft_full = false;
        perft_split = false;
        perft_cumulative = false;

        print_config = false;
        print_fen = false;
        print_ascii_pieces = false;
        
        app.parse(argc, argv);
    } 
    catch (const CLI::ParseError& e) {
        std::cerr << "info string cli invalid command" << std::endl;
    } 
    catch (const std::exception& e) {
        std::cout << "info string " << e.what() << std::endl;
    }
}

void Engine::handleUCI()
{
    std::cout << "id name Athena" << std::endl;
    std::cout << "id author Ariana Hejazyan" << std::endl;
    std::cout << "uciok" << std::endl << std::flush;
}

void Engine::handleIsReady()
{
    std::cout << "readyok" << std::endl << std::flush;
}

void Engine::handleSetOption()
{
    const auto& extras = app.get_subcommand("setoption")->remaining();

    if (extras.size() != 4 || extras[0] != "name" || extras[2] != "value") 
        throw std::invalid_argument("expected format: setoption name <name> value <value>");

    const std::string& name  = extras[1];
    const std::string& value = extras[3];

    if (name == "debug")
    {
             if (value == "on" ) debug = true ;
        else if (value == "off") debug = false;
        else throw std::invalid_argument("invalid debug value: " + value);
    }
    else throw std::invalid_argument("unknown option name: " + name);
}

void Engine::handleUCINewGame()
{
    // currently a stub
}

void Engine::handlePosition()
{
    auto* positionCommand = app.get_subcommand("position");
    std::vector<std::string> extras = positionCommand->remaining();
    
    std::string fen;
    if (position_mode == "fen") 
    {
        int numTokens = 7;
        if (extras.size() < numTokens)
            throw std::invalid_argument(
                "FEN requires " + std::to_string(numTokens) + " parameters, but received " + std::to_string(extras.size()));

        fen = concatenate(extras, 0, numTokens, ' ');
        extras.erase(extras.begin(), extras.begin() + numTokens);
    }
    else if (position_mode == "modern" ) fen = FEN_MODERN ;
    else if (position_mode == "classic") fen = FEN_CLASSIC;
    
    fromString(fen, pos);

    if (extras.empty()) return;

    if (extras[0] == "moves")
    {
        Move moves[MAX_MOVES];
        int size = 0;

        extras.erase(extras.begin());
        for (const auto& move : extras)
        {
            size = 0;
            size += genAllNoisyMoves(pos, moves + size);
            size += genAllQuietMoves(pos, moves + size);

            for (int i = 0; i < size; ++i)
            {
                if (toString(moves[i]) == move)
                {
                    pos.makemove(moves[i]);
                    break;
                }                
            }      
        }
    } 
    else { throw std::invalid_argument("expected 'moves' keyword"); }
}

void Engine::handleGo()
{
    int depth = 3;

    auto* goCommand = app.get_subcommand("go");
    const auto& extras = goCommand ? goCommand->remaining()
                                   : std::vector<std::string>{};

    // Parse optional "go depth N" from UCI input; default to depth 3 on error.
    for (size_t i = 0; i + 1 < extras.size(); ++i)
    {
        if (extras[i] == "depth")
        {
            try {
                depth = std::stoi(extras[i + 1]);
            } catch (...) {
                depth = 3;
            }
            break;
        }
    }

    using namespace std::chrono;

    // Start timing the search.
    auto start = steady_clock::now();

    // Fresh Thread: holds root move / score and node counter.
    Thread thread{};
    thread.score = 0;
    thread.move  = Move{};
    thread.nodes = 0;

    // Core search: full window [-SCORE_INFINITY, +SCORE_INFINITY]
    int score = negamax(pos, thread, -SCORE_INFINITY, SCORE_INFINITY, depth, 0);

    // Stop timing.
    auto end = steady_clock::now();
    auto ms  = duration_cast<milliseconds>(end - start).count();

    // Nodes and nodes-per-second (nps) from the Thread.
    std::uint64_t nodes = thread.nodes;
    std::uint64_t nps   = 0;
    if (ms > 0) {
        double seconds = ms / 1000.0;
        nps = static_cast<std::uint64_t>(nodes / seconds);
    }

    // UCI info: depth, score (cp), nodes, time (ms), nps, best move.
    std::cout << "info depth " << depth
              << " score cp " << score
              << " nodes " << nodes
              << " time " << ms
              << " nps " << nps
              << " pv " << toString(thread.move)
              << std::endl;
              
    std::cout << "bestmove " << toString(thread.move) << std::endl << std::flush;

}

void Engine::handleStop()
{
    // currently a stub
}

void Engine::handleQuit()
{
    std::exit(0);
}

void Engine::handlePerft()
{
    runPerftTests(pos, perft_depth, perft_full, perft_split, perft_cumulative);
}

void Engine::handlePrint()
{
    print(pos, print_fen, print_ascii_pieces);

    if (print_config)
    {
        std::cout << std::endl;
        std::cout << "configurations: "  << std::endl;
        std::cout << "debug " << (debug ? "on" : "off") << std::endl;
    }
}

void Engine::handleAthenaFen4()
{
    std::cout << "FEN4: 14/14/14/14/14/14/14/14/14/14/14/14/14/14" << std::endl << std::flush;
}

} // namespace athena
                            