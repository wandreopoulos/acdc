#include "Logger.h"
#include "BarnesHutSNEAdapter.h"
#include "MLUtil.h"

BarnesHutSNEAdapter::BarnesHutSNEAdapter()
{
}

BarnesHutSNEAdapter::~BarnesHutSNEAdapter()
{
}

unsigned BarnesHutSNEAdapter::estimateTsnePerplexity(const Eigen::MatrixXd & mat)
{
    return (unsigned) (pow(log(mat.rows()), 2)); 
}

double * BarnesHutSNEAdapter::loadData(const Eigen::MatrixXd & eigendata)
{
	int N = eigendata.rows();
	int D = eigendata.cols();
	double * data = (double *) malloc(D * N * sizeof(double));

	int nD = 0;
    for(int n = 0; n < N; n++) 
    {
        for(int d = 0; d < D; d++) 
        {
            data[nD + d] = eigendata(n, d);
        }
        nD += D;
    }

    return data;
}

Eigen::MatrixXd BarnesHutSNEAdapter::saveData(double* data, int N, int D)
{
	Eigen::MatrixXd eigendata(N, D);

	int nD = 0;
    for(int n = 0; n < N; n++) 
    {
        for(int d = 0; d < D; d++) 
        {
            eigendata(n, d) = data[nD + d];
        }
        nD += D;
    }

	return eigendata;	
}

// copied and modified from the original BH-SNE implementation
Eigen::MatrixXd BarnesHutSNEAdapter::runBarnesHutSNE(const Eigen::MatrixXd & eigendata, const Opts & opts)
{
    Eigen::MatrixXd tsneData;
    if (eigendata.cols() > opts.tsnePcaDim())
    {
        DLOG << "Initially reducing dimension to " << opts.tsnePcaDim() << " using PCA...\n";
        tsneData = MLUtil::pca(eigendata, opts.tsnePcaDim());
    } else
    {
        tsneData = eigendata;
    }

    TSNE* tsne = new TSNE();

    int N = tsneData.rows();
    int D = tsneData.cols();
    
    // estimate perplexity if necessary
    unsigned perplexity = opts.tsnePerplexity();
    if (perplexity == 0)
    {
        perplexity = BarnesHutSNEAdapter::estimateTsnePerplexity(tsneData);
    }

    // read the parameters and the dataset
	double * data = BarnesHutSNEAdapter::loadData(tsneData);
	
	// fire up the SNE implementation
	double * Y = (double*) malloc(N * opts.tsneDim() * sizeof(double));
    if (Y == NULL)
	{ 
		throw std::runtime_error("Memory allocation failed.");
	}
    srand(time(NULL));
    
    DLOG << "n=" << N << "   " 
            << "dim=" << D << "   " 
            << "targetDim=" << opts.tsneDim() << "   "
            << "perplexity=" << perplexity << "   "
            << "theta=" << opts.tsneTheta() << "\n";
	tsne->run(data, N, D, Y, opts.tsneDim(), perplexity, opts.tsneTheta());
	
	// save the results
	Eigen::MatrixXd result = BarnesHutSNEAdapter::saveData(Y, N, opts.tsneDim());
    
    // clean up
	free(data); 
	free(Y);
    
    delete tsne;

    return result;

}
