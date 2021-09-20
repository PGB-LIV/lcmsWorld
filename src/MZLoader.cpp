#include "MZLoader.h"



MZLoader::MZLoader()
{
}


MZLoader::~MZLoader()
{
}



signalFloat minSig = 3e27f;

SignalMz MZLoader::insertZeros(const std::vector<mzFloat> &mz, const std::vector<signalFloat> &sig)
{

	int size = mz.size();
	std::vector<mzFloat> newMz;
	std::vector<signalFloat> newSig;
	newMz.reserve(mz.size() * 3 / 2);
	newSig.reserve(mz.size() * 3 / 2);
	double minsig = 100;
	double gap_threshold = 3.5;



	newMz.push_back(mz[0] - 0.1f);
	newSig.push_back(0);


	for (int i = 0; i < size; i++)
	{

		mzFloat curMz = -1; //  mz[i];
		signalFloat curSig = sig[i];


		//flatten large gaps (more of a problem with bad noise removal)
 
		if (true && (i > 1) &&((mz[i] - mz[i - 1]) > gap_threshold))
		{
			auto lowsig = std::min(sig[i], sig[i - 1]);

			newMz.push_back(mz[i-1] + 0.01);
			newSig.push_back(lowsig);

			newMz.push_back(mz[i] - 0.01);
			newSig.push_back(lowsig);

		}
		else
		{



			if (1)
				if (i > 1)
				{
					if (mz[i] == mz[i - 1])
						continue;


					int j = i - 1;
					int j1 = i;

					mzFloat predMz = mz[j] + std::min((mz[j1] - mz[j]) * 3, 0.1f);


					if (curSig > minsig)
						if (predMz < mz[i])
						{
							newMz.push_back(predMz);
							newSig.push_back(0);
							curMz = predMz;
						}


				}



			if (1)
				if (i < size - 1)
				{
					int j = i;
					mzFloat predMz = mz[j] - std::min((mz[j + 1] - mz[j]) * 3, 0.1f);

					predMz = mz[j] - .1;

					//don't insert a new one behind the last one
					if (i > 0)
						if (curSig > minsig)
							if (predMz > curMz)
								if (predMz > mz[i - 1])
								{
									newMz.push_back(predMz);
									newSig.push_back(0);


								}

				}


		}
		
			
			



		newMz.push_back(mz[i]);
		newSig.push_back(curSig);



	}
	newMz.push_back(  mz[size - 1] + 0.1f);
	newSig.push_back(0);

	static float br = 0;
	float r = newMz.size()*100.0f / size;
	if (r > br)
	{
		br = r;
		// std::cout << br << "\n";
	}

	SignalMz result = { newMz,newSig };
	return result;
}


void MZLoader::addScan()
{
	// if both sets of mz & signal exist, then we have a scan!

//	std::cout << " add scan to " << result->id << " \n";


	if (mzData.size() != intensityData.size())
		std::cout << "SIZE ERROR! " << lcTime << "  " << mzData.size() << " " << intensityData.size() << std::endl;
	else
	{

		

		last_line = new MZScan(mzData, intensityData, lcTime);

		if (1)
		{


			SignalMz res = insertZeros(mzData, intensityData);
			delete last_line;
			last_line = new MZScan(res.mz, res.signal, lcTime);
		}

		result->append(last_line);


		lcount++;

	}

	mzData.clear();
	intensityData.clear();




}