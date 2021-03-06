#pragma once

#include <vector>
#include <string>
#include <json/json.h>

#include "ClusterAnalysis.h"
#include "KrakenAdapter.h"
#include "SequenceUtil.h"

// forward declaration
struct ClusteringResult;

// Contains the full result of one sample analysis
struct ResultContainer
{
	// Unique ID of the sample
	unsigned id;

	// Fasta file analyzed
	std::string fasta;

	// Contig names
	std::vector<std::string> fastaLabels;

	// One-shot cluster analysis
	ClusterAnalysisResult oneshot;

	// Bootstrap cluster analysis'
	std::vector<ClusterAnalysisResult> bootstraps;

	// Kraken results
	KrakenResult kraken;

	// 16S genes
	// _16S[i].empty() => no 16S gene found in oneshot data point i
	// otherwise       => 16S sequence _16s[i] found in oneshot data point i
	std::vector<std::string> _16S;

    // Statistics
    SequenceStats stats;
};

// Processing of result files (i.e. I/O, writing assets, JSON export, etc.)
class ResultIO
{

private:
	// Converts a vector of strings to a vector unsigned where each unique string corresponds to a unique unsigned value
	std::vector<unsigned> numericLabels(const std::vector<std::string> & labels);

	// Serialize a ClusteringResult to JSON
    Json::Value clusteringResultToJSON(const ClusteringResult & cr);

    // Serialize a ClusterAnalysisResult to JSON
    Json::Value clusterAnalysisResultToJSON(const ResultContainer & result, const ClusterAnalysisResult & car, bool alignToOneshot);

    // Serialize a full ResultContainer as JSON to filename
	void writeResultContainerToJSON(ResultContainer result, const std::string & filename);

	// Write 16S sequences
	void export16S(const ResultContainer & result);

	// Export fasta files for cluster exporting
    void exportClusteringInfo(const ResultContainer & result, const std::string & filename);

	void exportContigJS(const std::string & fastaFilename);

    // Output directory to write to
	std::string outputDir;

	// is Kraken enabled?
    bool krakenEnabled;

public:
	ResultIO(const std::string & outputDir, bool krakenEnabled);
	~ResultIO();

	// Processes a ResultContainer object to serialize it and write other information
	void processResult(const ResultContainer & result);

};
