#include "easylogging++.h"

#include "SequenceVectorizer.h"
#include "Util.h"

#include <string>
#include <vector>
#include <unordered_set>
#include <math.h>

#include <seqan/alignment_free.h> 
#include <seqan/sequence.h> 
#include <seqan/seq_io.h>

SequenceVectorizer::SequenceVectorizer(const unsigned kmerLength_, const unsigned windowWidth_, const unsigned windowStep_) : kmerLength(kmerLength_), windowWidth(windowWidth_), windowStep(windowStep_)
{
	buildFeatureKmers();
}


SequenceVectorizer::SequenceVectorizer(const Opts & opts)
{
	buildParams(opts);
	buildFeatureKmers();
}

SequenceVectorizer::~SequenceVectorizer() {}

void SequenceVectorizer::buildParams(const Opts & opts)
{
	inputFASTA = opts.inputFASTA();
	kmerLength = opts.windowKmerLength();
	windowWidth = opts.windowWidth();
	windowStep = opts.windowStep();

	if(windowWidth == 0)
	{
		// The file size in bytes is approximately the number of nucleotide (not accounting for ID strings)
		auto fileSize = Util::getFileSizeBytes(inputFASTA);
        windowStep = (unsigned) ceil((double)fileSize / (double)opts.targetNumPoints());
        windowWidth = 2 * windowStep;
        VLOG(2) << "k=" << opts.windowKmerLength() << "   "
                << "windowWidth=" << windowWidth << "   "
                << "windowStep=" << windowStep;
	}
	
	if(windowStep == 0)
	{
		windowStep = windowWidth / 2;
	}	
}

void SequenceVectorizer::buildFeatureKmers()
{
	unsigned idx = 0;
	for (auto kmer : Util::allKmers(kmerLength))
	{
		std::string revCompl = Util::reverseComplement(kmer);
		std::string & final = revCompl < kmer ? revCompl : kmer;

		if(kmerIndexes.find(final) != kmerIndexes.end())
		{
			kmerIndexes[kmer] = kmerIndexes[final];
		} else
		{
			kmerIndexes[kmer] = idx++;
		}

		uniqueKmers.insert(final);
	}	
}

Eigen::MatrixXd SequenceVectorizer::vectorize(seqan::Dna5String & sequence) const
{

	unsigned len = length(sequence);
	unsigned n = (unsigned) (((int)len - (int)windowWidth) / (int)windowStep) + 2;

	Eigen::MatrixXd mat = Eigen::MatrixXd::Zero(n, getDim());

	if (len < windowWidth)
	{
		VLOG(3) << "Length of contig is smaller than window size.";
		return mat;
	}

	for (unsigned i = 0; i < n; i++)
	{
		unsigned from = i*windowStep;
		unsigned to = std::min(len, i*windowStep+windowWidth);
		seqan::Infix<seqan::Dna5String>::Type window = infix(sequence, from, to);

		seqan::String<unsigned> counts;
		countKmers(counts, window, kmerLength);

		auto kmerIter = kmerIndexes.cbegin();
		for(const auto & cnt : counts)
		{
			std::string kmer = kmerIter->first;
			unsigned idx = kmerIter->second;
			mat(i, idx) += cnt;
			kmerIter++;
		}
	}

	if (normalize)
	{
		Eigen::MatrixXd m = mat.rowwise().sum().asDiagonal().inverse();
		mat = m * mat; 
	}

	return mat;
}

std::pair< Eigen::MatrixXd, std::vector<std::string> > SequenceVectorizer::vectorize() const
{
	if(inputFASTA.empty())
	{
		throw std::runtime_error("Input FASTA not set! Use Opts constructor.");
	}

	seqan::SeqFileIn seqFileIn(inputFASTA.c_str());
	seqan::StringSet<seqan::CharString> ids;
	seqan::StringSet<seqan::String<seqan::Iupac> > seqs;

	readRecords(ids, seqs, seqFileIn);

	unsigned n = length(ids);

	Eigen::MatrixXd result(0, getDim());
	std::vector<std::string> labels;

	for (unsigned i = 0; i < n; i++)
	{
		std::string id;
		move(id, ids[i]);
		seqan::Dna5String seq = seqs[i];

		auto mat = vectorize(seq);
		unsigned rows = mat.rows();

		// concatenate matrices TODO that's quite expensive
		auto tmp = result;
		result = Eigen::MatrixXd(tmp.rows() + mat.rows(), getDim());
		result << tmp, mat;
		
		for (unsigned j = 0; j < rows; j++)
		{
			labels.push_back(id);
		}
	}
	
	return std::make_pair(result, labels);
}

unsigned SequenceVectorizer::getDim() const { return uniqueKmers.size(); }
std::set<std::string> SequenceVectorizer::getFeatures() const { return uniqueKmers; }
unsigned SequenceVectorizer::getKmerLength() const { return kmerLength; }
unsigned SequenceVectorizer::getWindowWidth() const { return windowWidth; }
unsigned SequenceVectorizer::getWindowStep() const { return windowStep; }
void SequenceVectorizer::setNormalize(const bool normalize_) { normalize = normalize_; }
bool SequenceVectorizer::getNormalize() const { return normalize; }
