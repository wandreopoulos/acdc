#include "easylogging++.h"
INITIALIZE_EASYLOGGINGPP

#include "SequenceVectorizer.h"
#include "BarnesHutSNEBridge.h"
#include "Util.h"
#include "Opts.h"

#include <iostream>

// logging: VERBOSE 1: progress
//          VERBOSE 2: parameters and more than progress
//          VERBOSE 3: t-SNE and more

int main(int argc, char const *argv[])
{
	START_EASYLOGGINGPP(argc, argv);

	std::unique_ptr<Opts> opts;

	try 
	{
		opts.reset(new Opts(argc, argv));
	}
	catch(const std::exception & e) 
	{
		std::cerr << e.what() << '\n';
		return EXIT_FAILURE;
	}

	if (opts->needsHelp())
	{
		std::cout << opts->helpDesc() << std::endl;
		return EXIT_SUCCESS;
	}

	if (opts->inputFASTA().empty())
	{
		std::cerr << "No input FASTA file given (--input-fasta,-i), aborting." << std::endl;
		return EXIT_FAILURE;
	}


	VLOG(1) << "Vectorizing contigs...";
	SequenceVectorizer sv(*opts);
	auto dat = sv.vectorize();

	VLOG(1) << "Running t-SNE...";
	auto reduced = BarnesHutSNEBridge::runBarnesHutSNE(dat.first, *opts);

	Util::saveMatrix(reduced, "/tmp/reduced", ' ');

	return EXIT_SUCCESS;
}