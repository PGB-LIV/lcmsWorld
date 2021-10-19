
#include "MZData.h"
#include "Landscape.h"
#include "MZScan.h"
#include "Tile.h"
#include "Zip.h"
#include "Builder.h"
#include <iostream>  
#include <algorithm>
#include <set>
#include <sstream>
#include <mutex>
#include "Cache.h"


#if 0
double MZData::xScale;
double MZData::yScale;
double MZData::zScale;

worldView MZData::viewData = { 3500,80000,30000 };
#endif



int numMZData = 0;
const int bucketsPerTile = 128;




//	std::vector<byte> buffer;
MZData::MZData()
{
	numMZData++;

	id = next_id++;

	info.lcRange.set = false;
}

int MZData::next_id = 0;


std::vector<signalFloat>  rleEncode(const std::vector<signalFloat>& data)
{
	if (Cache::makeMetaFile)
		return data;


	std::vector<signalFloat> new_buffer;
	new_buffer.reserve(data.size());
	float numZeros = 0;
	for (auto sig : data)
	{
		if (sig == 0)
		{
			if (numZeros > 0)
			{
				numZeros++;

			}
			else
			{
				new_buffer.push_back(0);
				numZeros++;
			}
		}
		else
		{
			if (numZeros > 0)
				new_buffer.push_back(numZeros);
			numZeros = 0;
			new_buffer.push_back(sig);
		}
		if (new_buffer.size() >= data.size())
			return data;
	}

	if (numZeros > 0)
		new_buffer.push_back(numZeros);

	//	std::cout << new_buffer.size() - data.size() << "   " << new_buffer.size() << "\n";
	if (new_buffer.size() < data.size())
		return new_buffer;

	return data;
}


std::vector<signalFloat>  rleDecode(const std::vector<signalFloat>& data, int dest_size)
{

	std::vector<signalFloat> new_buffer;
	new_buffer.reserve(dest_size);
	bool foundZero = false;
	for (auto sig : data)
	{
		if (foundZero)
		{
			for (int i = 0; i < sig; i++)
				new_buffer.push_back(0);
			foundZero = false;
		}
		else
		{
			if (sig == 0)
			{
				foundZero = true;
			}
			else
			{
				new_buffer.push_back(sig);
			}
		}

	}

	return new_buffer;


}


/* Raw file format is :-
info (range)
number of scans (int)
number of mz scans (int) (typically 1 or all)
for each scan:
lctime (double)
number of mz entries (int) may be 0
for each mz entry
value (mzfloat)
number of intensity entries (int)
size of data (int) - only different if some encoding is used
for each intensity entry
value (signalFloat)

*/
//




//obviously, this must match the format of serialise exactly...

void MZData::deSerialise(const std::vector<byte>& buffer)
{
	// std::cout << " deSerialise " << buffer.size() << "\n";
	if (buffer.size() == 0)
	{
		// load error
		//todo - some sort of error reporting
		return;
	}
	const byte* ptr = &buffer[0];
	memcpy(&info, ptr, sizeof(info));
	ptr += sizeof(info);

	int num_scans;

	memcpy(&num_scans, ptr, sizeof(num_scans));
	ptr += sizeof(num_scans);

	info.num_points = 0;
	int num_mz;
	memcpy(&num_mz, ptr, sizeof(num_mz));
	ptr += sizeof(num_mz);

	if (num_mz > 1)
		type = MZDataType::jagged;
	else
		type = MZDataType::square;

	for (int i = 0; i < num_scans; i++)
	{


		double lc_time;
		memcpy(&lc_time, ptr, sizeof(lc_time));
		ptr += sizeof(lc_time);

		int mz_size;
		memcpy(&mz_size, ptr, sizeof(mz_size));
		ptr += sizeof(mz_size);
		std::vector<mzFloat> mz_data;
		if (mz_size > 0)
		{
			mzFloat* mz_buffer = (mzFloat*)ptr;
			mz_data.assign(mz_buffer, mz_buffer + mz_size);
			ptr += sizeof(mzFloat) * mz_size;
		}




		int signal_size;
		memcpy(&signal_size, ptr, sizeof(signal_size));
		ptr += sizeof(signal_size);
		int signal_buffer_size;
		memcpy(&signal_buffer_size, ptr, sizeof(signal_buffer_size));
		ptr += sizeof(signal_buffer_size);

		// if the signal and buffer size are different, then the data is encoded
		//allocate signal_buffer_size to the array, but only read signal_size bytes

		std::vector<signalFloat> signal_data;


		if (signal_size > 0)
		{
			// different sizes denotes it's encoded
			if (signal_buffer_size != signal_size)
			{
				//need to decode the buffer
				std::vector<signalFloat> buffer((signalFloat*)ptr, (signalFloat*)ptr + signal_buffer_size);
				auto decoded = rleDecode(buffer, signal_size);
				if (decoded.size() != signal_size)
				{
					std::cout << " decoded size error " << decoded.size() << "  " << signal_size << "\n";
				}
				signalFloat* signal_buffer = &decoded[0];
				signal_data.assign(signal_buffer, signal_buffer + signal_size);


				ptr += sizeof(signalFloat) * signal_buffer_size;
				info.num_points += signal_size;
			}
			else
			{
				signalFloat* signal_buffer = (signalFloat*)ptr;
				signal_data.assign(signal_buffer, signal_buffer + signal_size);
				ptr += sizeof(signalFloat) * signal_size;
				info.num_points += signal_size;
			}
		}
		MZScan* scan_p;
		if (mz_size > 0)
		{
			scan_p = new MZScan(mz_data, signal_data, (lcFloat)lc_time);
		}
		else
		{
			scan_p = new MZScan(signal_data, (lcFloat)lc_time);
		}
		scans.push_back(scan_p);

	}
	// std::cout << " added " << scans.size() << "\n";
}







std::vector<std::vector<byte >> buffers;

void MZData::cleanUp()
{

	while (buffers.size() > 0)
	{
		buffers.pop_back();
	}
	buffers.clear();
	std::vector<std::vector<byte >>().swap(buffers);

}

const std::vector<byte> MZData::serialise(int thread, bool compress)
{

	return serialise(thread);

	// no longer used - it's now done at putfile / getfile level
	std::vector<byte> data = serialise(thread);


	int bufferSize = Zip::GetMaxCompressedLen(data.size());
	std::vector<byte> compressed_buffer;
	compressed_buffer.resize(bufferSize);
	int len = Zip::CompressData(&data[0], data.size(), &compressed_buffer[0], bufferSize);
	compressed_buffer.resize(len);
	//	std::cout << " was " << data.size() << " now " << len << "  " << (len*100.0 / data.size()) << "\n";
	return compressed_buffer;
}
std::mutex s_lock;



const std::vector<byte> MZData::serialise(int thread)
{
	const int buffer_size = 1024 * 1024 * 4 * 4;

	s_lock.lock();

	while (buffers.size() < thread + 1)
	{
		std::vector<byte> abuffer;
		buffers.push_back(abuffer);
		buffers[buffers.size() - 1].resize(buffer_size);

	}


	s_lock.unlock();


	//todo fix buffers[thread] size to be depend on data
	//too large  abuffers[thread] takes ages to allocate

//	if (buffers[thread].size() == 0)
//		buffers[thread].resize(1024 * 128 * 20);

	//	if (size() == 0)
	//	return buffers[thread];


	setRange();

	int ptr = 0;

	memcpy(&buffers[thread][ptr], &info, sizeof(info));
	ptr += sizeof(info);


	int num_scans = (int)scans.size();
	memcpy(&buffers[thread][ptr], &num_scans, sizeof(num_scans));
	ptr += sizeof(num_scans);


	int num_mz = num_scans;
	if (type == MZDataType::square)
		num_mz = 1;

	memcpy(&buffers[thread][ptr], &num_mz, sizeof(num_mz));
	ptr += sizeof(num_mz);

	int scan_num = 0;



	for (auto scan : scans)
	{



		double lc_time = scan->getLcTime();

		memcpy(&buffers[thread][ptr], &lc_time, sizeof(lc_time));
		ptr += sizeof(lc_time);

		//write mz buffers[thread]
		if (scan_num < num_mz)
		{
			int scan_size = (int)scan->getMz().size();
			assert(scan_size > 0);
			memcpy(&buffers[thread][ptr], &scan_size, sizeof(scan_size));
			ptr += sizeof(scan_size);



			const mzFloat* data_ptr = &scan->getMz().front();
			memcpy(&buffers[thread][ptr], data_ptr, sizeof(mzFloat) * scan_size);
			ptr += sizeof(mzFloat) * scan_size;
		}
		else
		{
			int scan_size = 0;
			memcpy(&buffers[thread][ptr], &scan_size, sizeof(scan_size));
			ptr += sizeof(scan_size);
		}

		// now write intensity buffers[thread]
		int scan_size = (int)scan->getIntensity().size();


		assert(scan_size > 0);

		auto rle_buffer = rleEncode(scan->getIntensity());
		memcpy(&buffers[thread][ptr], &scan_size, sizeof(scan_size));
		ptr += sizeof(scan_size);
		signalFloat* data_ptr = &rle_buffer[0];
		int scan_buffer_size = rle_buffer.size();

		memcpy(&buffers[thread][ptr], &scan_buffer_size, sizeof(scan_buffer_size));
		ptr += sizeof(scan_buffer_size);

		memcpy(&buffers[thread][ptr], data_ptr, sizeof(signalFloat) * scan_buffer_size);
		ptr += sizeof(signalFloat) * scan_buffer_size;
		assert(ptr < buffers[thread].size());

		if (ptr > buffers[thread].size() * 3 / 4)
		{
			buffers[thread].resize(buffers[thread].size() * 4 / 3);

		}
		scan_num++;
	}


	// 	std::vector<byte>().swap(buffers[thread]);


	std::vector<byte> new_buffer;
	new_buffer.resize(ptr);
	memcpy(&new_buffer[0], &buffers[thread][0], sizeof(byte) * ptr);

	// s_lock.unlock();
	return new_buffer;
}

//should be same as above function
const int MZData::serialise(byte* buffer)
{
	return -1;
}






void MZData::append(MZScan* scan)
{




	if ((scan->parent != this) && (scan->parent != NULL))
	{
		std::cout << "scan already used...\n" << scan->parent;
		int y = 4;
		std::cout << y;
	}
	scans.push_back(scan);
	info.num_points += (int)scan->getIntensity().size();
	//	info.lcRange.set = false;
}

std::ostream& operator<<(std::ostream& os, const MZData& dt)
{
	os << dt.scans.size() << "  " << dt.info.lcRange.min << " " << dt.info.lcRange.max << " mz " << dt.info.mzRange.min << " " << dt.info.mzRange.max << std::endl;

	return os;
}

std::string MZData::getInfoString()
{
	std::ostringstream out;
	out << info.lcRange.min << " , " << info.mzRange.min << " , " << info.lcRange.max << " , " << info.mzRange.max;
	return out.str();
}
double MZData::setRange(bool setIntensity)
{
	double sum = 0;

	if (setIntensity == false)
		if (info.lcRange.set)
			return signalSum;
	if (scans.size() == 0)
		return 0;

	double last = -1;
	for (auto s : scans)
	{
		if (s->getLcTime() < last)
		{
			std::cout << " ERROR " << s->getLcTime() << " < " << last << "\n";
			Builder::error = Builder::errorType::nonSeqMzml;
		}
		last = s->getLcTime();

	}

	info.lcRange.min = scans.front()->getLcTime();
	info.lcRange.max = scans.back()->getLcTime();
	info.lcRange.set = true;

	Range<mzFloat> mzRangeTemp = { 3e27f,0 };

	if (scans.front() == NULL)
	{
		std::cout << " scan is null \n";
		return 0;
	}

	if (scans.front()->getMz().size() > 0)
	{
		mzRangeTemp.min = scans.front()->getMz().front();
		mzRangeTemp.max = scans.front()->getMz().back();
	}
	else
	{
		std::cout << " scan has no mz \n";
	}

	info.num_points = 0;


	//	if (type == jagged)
	{
		for (auto scan : scans)
		{
			if (scan->getMz().size() > 0)
			{


				mzFloat mzMin = scan->getMz().front();
				mzFloat mzMax = scan->getMz().back();
				mzRangeTemp.min = std::min(mzMin, mzRangeTemp.min);
				mzRangeTemp.max = std::max(mzMax, mzRangeTemp.max);


			}
			//rescanning is a bit inefficient after a merge or append
			//but leave it for now, rebuilding meshes  probably makes it insignificant!

			auto intensity = (scan->getIntensity());
			maxSize = std::max(maxSize, (int)intensity.size());

			info.num_points += (int)intensity.size();

			//only do this once (should do it for smaller tiles, but not sure how at the moment)
		//	if (setIntensity)
			for (auto i : intensity)
			{
				//if (i > 0)

				sum += i;
				info.signalRange.min = std::min(i, info.signalRange.min);
				info.signalRange.max = std::max(i, info.signalRange.max);



				if (info.signalRange.max > 1e37)
				{
					std::cout << "too large data found \n";
				}
			}



		}
	}

	mzRangeTemp.set = true;

	info.mzRange = mzRangeTemp;


	signalSum = sum;
	return sum;
}

//add the new object to this one
//useful for building up a complete dataset in parts
//so to make the complete summary, I read in 'n' lines, reduce it, then append to MZData object
//This does not check they are in the right order! (of LCtime)

//in future - can it regenerate GLMesh on the fly? (not sure it is useful or desirable)
void MZData::merge(MZData* o)
{
	auto newScans = o->getScans();
	for (auto scan : newScans)
	{
		append(scan);
	}

	setRange();
}

std::vector<DataPoint> findClosestinLine(DataPoint s, MZScan* line, MZScan* base)
{
	std::vector<DataPoint> found;
	if (line->getSize() == 0)
		return found;


	auto useMz = base->getMz();
	if (line->getMz().size() > 0)
		useMz = line->getMz();

	auto lastMz = useMz[0];

	auto intensity = line->getIntensity();

	int size = useMz.size();
	double range = .15;
	do
	{
		found.clear();
		auto minMz = s.mz - range;
		auto maxMz = s.mz + range;


		int left = 0;
		int right = size;

		int i = 0;
		//now does a binary search
		int mid = (left + right) / 2;
		while ((right - left) > 1)
		{
			if (useMz[mid] < minMz)
				left = mid;
			else
				right = mid;

			mid = (left + right) / 2;
			i++;
		}

		mid = std::max(mid - 1, 0);

		while (mid < size)
		{
			auto mz = useMz[mid];
			if (mz > minMz)
			{
				auto signal = intensity[mid];
				if (signal > 0)
				{
					DataPoint next = { mz, line->getLcTime(), signal };
					/*
					if (mid < useMz.size() - 1)
						next.mz2 = useMz[mid + 1];
					else
						if (mid > 0)
							next.mz2 = next.mz + (useMz[mid] - useMz[mid - 1]);
						else
							next.mz2 = next.mz + .5;
					 */

					found.push_back(next);
				}
				if (mz > maxMz)
					break;
			}
			mid++;
			i++;
			lastMz = mz;
		}
		range *= 1.5;
	} while ((found.size() < 2) && (range < 32));
	return found;

}

std::vector<DataPoint> MZData::findClosePointsinLine(DataPoint s, MZScan* line, MZScan* base)
{
	std::vector<DataPoint> found;
	if (line->getSize() == 0)
		return found;


	auto useMz = base->getMz();
	if (line->getMz().size() > 0)
		useMz = line->getMz();

	auto lastMz = useMz[0];


	double range = 1.5;


	found.clear();
	auto minMz = s.mz - range;
	auto maxMz = s.mz + range;

	int i = 0;
	for (auto mz : useMz)
	{

		if (mz > minMz)
		{
			DataPoint next = { mz, line->getLcTime(), line->getIntensity()[i] };
			if (next.signal > 0)
				found.push_back(next);
			if (mz > maxMz)
				break;
		}

		i++;
		lastMz = mz;
	}

	range *= 1.5;
	return found;

}

//getting a whole load of points - to be used in creating 3d screenshots
std::vector<DataPoint>  MZData::findClosePoints(DataPoint s)
{
	std::vector<DataPoint> found;


	//should maybe find a way of signalling that nothing was found
	//s.signal  < 0 ?
	if (scans.size() == 0)
		return found;

	//lc range 
	double range = 15;


	found.clear();
	auto minLc = s.lc - range;
	auto maxLc = s.lc + range;


	auto lastScan = scans[0];
	for (auto scan : scans)
	{
		if (scan->getLcTime() > minLc)
		{
			auto add = findClosestinLine(s, scan, scans[0]);
			for (auto p : add)
				found.push_back(p);


			if (scan->getLcTime() > maxLc)
				break;
		}
		lastScan = scan;
	}



	return found;

}

std::vector<DataPoint>  MZData::findClosest(DataPoint s)
{
	std::vector<DataPoint> found;


	//should maybe find a way of signalling that nothing was found
	//s.signal  < 0 ?
	if (scans.size() == 0)
		return found;

	double range = .5;
	do
	{
		found.clear();
		auto minLc = s.lc - range;
		auto maxLc = s.lc + range;


		auto lastScan = scans[0];
		int i = 0;
		int size = scans.size();
		for (auto scan : scans)
		{
			i++;
			if (scan->getLcTime() > minLc)
			{
				auto add = findClosestinLine(s, scan, scans[0]);
				for (auto p : add)
				{
					/*
					p.lc2 = p.lc+0.5;
					if (i < size)
						p.lc2 = scans[i]->getLcTime();
						*/

					found.push_back(p);
				}


				if (scan->getLcTime() > maxLc)
					break;
			}
			lastScan = scan;
		}
		range *= 1.5;

	} while ((found.size() < 10) && (range < 52));

	return found;
#if 0
	mzFloat closestMz;
	lcFloat closestLc;
	DataPoint result = s;
	double closestDistance = std::numeric_limits<double>::max();


	for (auto p : found)
	{
		double dx = p.mz - s.mz;
		double dy = p.lc - s.lc;
		dx *= MZData::xScale;
		dy *= MZData::yScale;



		double dz = p.signal + s.signal;
		dz *= MZData::zScale;
		dz = Mesh::convertZ(dz);

		dx *= Settings::scale.x;
		dy *= Settings::scale.y;
		dz *= Settings::scale.z;

		double dist = (dx * dx) + (dy * dy) + (dz * dz);
		if (dist < closestDistance)
		{
			closestDistance = dist;
			result = p;
		}
	}


	std::cout << s.mz << "  " << s.lc << "  , " << result.mz << " , " << result.lc << "   " << result.signal << "  : " << found.size() << " \n ";

	return result;
#endif

}

/*
std::vector<MZData*> & MZData::split(int xsize, int ysize)
{
	setRange();

}
*/

// for ragged arrays, this could produce weird effects
// worry about later!

std::vector<MZData*> MZData::new_split(int xsize, int ysize)
{
	std::vector<MZData*> newData;
	MZData* table[32][32];
	assert(xsize <= 32);
	assert(ysize <= 32);

	for (int i = 0; i < xsize; i++)
		for (int j = 0; j < ysize; j++)
		{
			MZData* newMz = new MZData();

			newData.push_back(newMz);
			table[i][j] = newMz;
		}

	int num_scans = scans.size();
	std::vector<mzFloat> ends;
	ends.resize(xsize);

	//setup the buckets

	int buckets[256 * 2];
	for (int i = 0; i < 256; i++)
		buckets[i] = 0;
	long long total = 0;
	for (int i = 0; i < num_scans; i++)
	{
		double min = this->info.mzRange.min;
		double max = this->info.mzRange.max;
		MZScan* scan = scans[i];
		auto mzs = scan->getMz();
		for (auto mz : mzs)
		{
			int bucket = (int)(((mz - min) * 256) / (max - min));

			if (bucket < 0)
			{
				bucket = 0;
				std::cout << " Error - mz less than min, check ordering of points " << min <<" " << mz << " \n";

			}
			if (bucket > 255)
				bucket = 255;

			buckets[bucket]++;
			total++;
		}
	}



	for (int i = 0; i < xsize; i++)
	{
		long long find = ((i + 1) * total) / xsize;

		long long sum = 0;
		long long cur_bucket = 0;
		while (sum < find)
		{
			sum += buckets[cur_bucket];
			cur_bucket++;
		}
		mzFloat min = this->info.mzRange.min;

		mzFloat max = this->info.mzRange.max;

		ends[i] = min + ((cur_bucket - 0.5f) * (max - min) / 256);
	 //	std::cout << " set end " << i << "  " << ends[i] << "  at bct " << cur_bucket << " with sum " << sum << " < " << find << "\n";

	}

	ends[xsize - 1] = this->info.mzRange.max;

	for (int i = 0; i < num_scans; i++)
	{
		int ypos = i * ysize / num_scans;
		//need to split each line up into 
		MZScan* scan = scans[i];
		scans[i] = NULL;

		int scanSize = scan->getSize();

		auto intensity = scan->getIntensity();
		auto mz = scan->getMz();

		int xpos = 0;
		int mzSize = mz.size();

		signalFloat last_signal = 0;
		mzFloat last_mz = mz[1] - 0.1f;

		for (int j = 0; j < xsize; j++)
		{
			MZScan* part;
			if (scanSize > 0)
			{

				int startPos = j * scanSize / xsize;
				int endPos = (j + 1) * scanSize / xsize - 1;


				std::vector<signalFloat> signal_vals;
				std::vector<mzFloat> mz_vals;
				signal_vals.reserve(mzSize + 1);
				mz_vals.reserve(mzSize + 1);

				if (mzSize == 0)
				{

					if (mzSize > 0)
					{
						//	ends[j] = mz[endPos];
						std::vector<mzFloat> mz_vals_copy(&mz[startPos], &mz[endPos]);
						mz_vals = mz_vals_copy;
					}

					std::vector<signalFloat> signal_vals_copy(&intensity[startPos], &intensity[endPos]);
					signal_vals = signal_vals_copy;
				}
				else
				{
					if (j > 0)
					{
						//		signal_vals.push_back(last_signal );

						//		mz_vals.push_back(last_mz  );

					}
					while ((mz[xpos] < ends[j]) && (xpos < scanSize - 1))
					{

						signal_vals.push_back(intensity[xpos]);
						mz_vals.push_back(mz[xpos]);

						xpos++;



					}

					//small amount of tile overlap should be ok...
					if (xpos > 0)
						if ((xpos < mz.size()) && (mz[xpos] - mz[xpos - 1] < 0.5))
						{
							signal_vals.push_back(intensity[xpos]);
							mz_vals.push_back(mz[xpos]);
						}

				}


 
				//if row is empty, fill in some blanks
				if (signal_vals.size() < 2)
				{
		 
					if (signal_vals.size() == 0)
					{
						signal_vals.push_back(0);
						if (j > 0)
							mz_vals.push_back(ends[j - 1]);
						else
							mz_vals.push_back(ends[j] - 1);
					}
					signal_vals.push_back(0);
					mz_vals.push_back(ends[j]);

					//it can be marginally over the end of the split
					if (mz_vals[1] < mz_vals[0])
					{
						auto t1 = mz_vals[1];
						mz_vals[1] = mz_vals[0];
						mz_vals[0] = t1;
			 
					}
					


				}
		 

		

				last_signal = signal_vals.back();
				last_mz = mz_vals.back();
				//now got a partical scan

				if (scan->getMz().size() > 0)
				{
					part = new MZScan(mz_vals, signal_vals, scan->getLcTime(), table[j][ypos]);

				}
				else
				{
					part = new MZScan(signal_vals, scan->getLcTime(), table[j][ypos]);
				}

			}
			else
			{
				part = new MZScan(scan);

			}
			// split_buffer.insert((long long) part);
			
							table[j][ypos]->append(part);


			//duplicate scans across parts, to create a little overlap

			if (ypos > 0)
				if (table[j][ypos]->size() < 3)
				{

					auto part2 = new MZScan(part);
					table[j][ypos - 1]->append(part2);
				}

		}


		delete scan;


	}



	scans.clear();
	std::vector<MZScan*>().swap(scans);

	/*
		std::cout << " split " << xsize << " , " << ysize << " : " << getScans().size() << "  \n";
		for (auto part : newData)
		{
			part->setRange();
			std::cout << part->getScans().size() << "  ";
		}
		std::cout << "\n";

		*/
	for (auto part : newData)
		part->setRange();
	return newData;

}


// std::vector<MZData*> MZData::split(int xsize, int ysize)
std::vector<MZData*> MZData::old_split(int xsize, int ysize)
{

	setRange();
	std::vector<MZData*> newData;
	newData.reserve(xsize * ysize);

	//start of bucket
	std::vector<mzFloat> mzbuckets(xsize + 1);



	for (int i = 0; i < xsize; i++)
	{
		mzbuckets[i] = (mzFloat)(info.mzRange.min + (((info.mzRange.max - info.mzRange.min) / xsize) * (i + 1)));


	}

	mzbuckets[xsize - 1] = info.mzRange.max;
	std::vector<lcFloat> lcbuckets(ysize + 1);

	//midpoint

	for (int i = 0; i < ysize; i++)
	{
		lcbuckets[i] = info.lcRange.min + (((info.lcRange.max - info.lcRange.min) / ysize) * (i + 1));


	}
	lcbuckets[ysize - 1] = info.lcRange.max;
	mzbuckets[xsize] = std::numeric_limits<mzFloat>::max();
	lcbuckets[ysize] = std::numeric_limits<lcFloat>::max();

	int height = (int)scans.size();
	int y = 0;




	// there are x lists (one for each tile across),

	// each becomes an MZScan object
	//so we need y lists of MZcvans
	//at the end of a tile
	std::vector<std::vector<MZScan*>> lineScans(xsize, std::vector<MZScan*>());

	std::vector<std::vector<MZScan*>> scanBuffer(ysize, std::vector<MZScan*>());
	//tilebuffer
	for (int i = 0; i < height; i++)
	{
		MZScan* line = scans[i];
		while (line->getLcTime() > lcbuckets[y])
		{
			//move on to the next LC line
				//
			if (lineScans[0].size() > 0)
				for (int j = 0; j < xsize; j++)
				{

					if (lineScans[j].size() > 0)
					{
						auto tilescans = lineScans[j];

						MZData* tileData = new MZData();

						for (auto scan : tilescans)
						{
							tileData->append(scan);
						}
						tileData->setRange();

						tileData->copyRange(this);

						if (tileData->getScans().size() > 0)
							newData.push_back(tileData);

						if (lineScans[j].size() > 0)
						{
							//copying last line to duplicate points between tiles
							auto last = lineScans[j].back();
							lineScans[j].pop_back();

							//deleteing scans?

							for (auto data : lineScans[j])
							{
								delete data;

							}

							lineScans[j].clear();
							lineScans[j].push_back(last);
						}


					}
				}

			y++;
		}
		auto mzData = line->getMz();
		if (type == MZDataType::square)
			mzData = scans[0]->getMz();

		auto intData = line->getIntensity();


		int x = 0;

		//		std::vector<MZScan*> lineScans(xsize);

		std::vector<mzFloat> lineMz;
		std::vector<signalFloat> lineSignal;

		for (int j = 0; j < mzData.size(); j++)
		{
			while (mzData[j] > mzbuckets[x])
			{

				if (lineSignal.size() > 2)
				{


					MZScan* scan = new MZScan(lineMz, lineSignal, line->getLcTime());
					lineScans[x].push_back(scan);


					auto lastMz = lineMz.back();
					auto lastSignal = lineSignal.back();
					lineMz.pop_back();
					lineSignal.pop_back();
					auto lastMz2 = lineMz.back();
					auto lastSignal2 = lineSignal.back();

					lineMz.clear();



					lineSignal.clear();
					lineMz.push_back(lastMz2);
					lineSignal.push_back(lastSignal2);
					lineMz.push_back(lastMz);
					lineSignal.push_back(lastSignal);
				}

				x++;
			}
			lineMz.push_back(mzData[j]);
			lineSignal.push_back(intData[j]);

		}
		if (lineSignal.size() > 0)
		{

			MZScan* scan = new MZScan(lineMz, lineSignal, line->getLcTime());
			lineScans[x].push_back(scan);
		}



	}


	//clean up the last set of y scans
	if (lineScans[0].size() > 0)
		for (int j = 0; j < xsize; j++)
		{
			auto tilescans = lineScans[j];

			MZData* tileData = new MZData();
			for (auto scan : tilescans)
			{
				tileData->append(scan);
			}
			tileData->setRange();

			tileData->copyRange(this);

			if (tileData->getScans().size() > 0)
				newData.push_back(tileData);


			lineScans[j].clear();



		}

	//	std::cout << "split x,y " << xsize << " , " << ysize << "\n";

	for (auto tile : newData)
	{

		tile->setRange();

		// std::cout << tile->info.lcRange.min << " " << tile->info.lcRange.max << " " << tile->info.mzRange.min << " " << tile->info.mzRange.max  <<"  " << tile->getScans().size() << "\n";
	}
	return newData;
}


int MZData::reduceType = 1;
#define reduceType 1

MZData* MZData::reduce(int xsize, int ysize)
{

	setRange();
	//start of bucket



	double sum = 0;
	//midpoint
	std::vector<mzFloat> mzbuckets(xsize + 1);
	for (int i = 0; i < xsize; i++)
	{
		mzbuckets[i] = info.mzRange.min + (((info.mzRange.max - info.mzRange.min) * (i)) / xsize);


	}
	if (0)
		if (xsize >= scans.front()->getMz().size() * 2)
			xsize = scans.front()->getMz().size() * 2;

	//todo
	// in this case, we may need to make it a jagged array 
	if (0)
		if (xsize > scans.front()->getMz().size())
		{

			//don't resize in x dimension - we will use original points
			//force copy of vector
			std::vector<mzFloat> temp(scans.front()->getMz());
			// temp.push_back(info.mzRange.max);
			mzbuckets = temp;
			xsize = mzbuckets.size();

		}


	mzbuckets[xsize - 1] = info.mzRange.max;
	std::vector<lcFloat> lcbuckets(ysize + 1);

	//midpoint

	for (int i = 0; i < ysize; i++)
	{
		lcbuckets[i] = info.lcRange.min + (((info.lcRange.max - info.lcRange.min) * (i)) / (ysize));
	}


	// don't resize in y dimension
	if (0)
		if (ysize >= scans.size())
		{
			lcbuckets.resize(scans.size());
			int i = 0;
			for (auto scan : scans)
				lcbuckets[i++] = scan->getLcTime();

			//		lcbuckets[i++] = info.lcRange.max;
		}


	size_t height = scans.size();

	if (true)
	{
		lcbuckets[ysize - 1] = std::numeric_limits<lcFloat>::max();
		// intensityVals[j] = data[i*size + j];
		std::vector<int> removelcbuckets;

		for (int i = 0; i < lcbuckets.size() - 1; i++)
		{
			int between = 0;
			for (auto scan : scans)
			{
				bool t = lcbuckets[i] < scan->getLcTime();
				bool b = lcbuckets[i + 1] < scan->getLcTime();
				if (t != b)
					between++;
			}
			if (between == 0)
			{
				removelcbuckets.push_back(i);


			}
		}



		std::sort(removelcbuckets.begin(), removelcbuckets.end());

		std::vector<lcFloat> newBuckets;

		for (int i = 0; i < lcbuckets.size(); i++)
		{
			if (!std::binary_search(removelcbuckets.begin(), removelcbuckets.end(), i))
				newBuckets.push_back(lcbuckets[i]);
		}

		ysize = (int)newBuckets.size() - 1;

		lcbuckets.clear();
		for (auto i : newBuckets)
			lcbuckets.push_back(i);
	}


	lcbuckets[0] = info.lcRange.min;


	lcbuckets[ysize - 1] = info.lcRange.max;
	mzbuckets[xsize] = std::numeric_limits<mzFloat>::max();
	lcbuckets[ysize] = std::numeric_limits<lcFloat>::max();


	// intensityVals[j] = data[i*size + j];


	std::vector<signalFloat> counts((xsize + 1) * (ysize + 1));

	std::vector<signalFloat> data((xsize + 1) * (ysize + 1));

	for (int i = 0; i < xsize + 1; i++)
		for (int j = 0; j < ysize + 1; j++)
			data[j * xsize + i] = 0;

	for (int i = 0; i < xsize + 1; i++)
		for (int j = 0; j < ysize + 1; j++)
			counts[j * xsize + i] = 0;
	int y = 0;
	int max = 0;
	lcFloat lastYpos = 0;

	for (int i = 0; i < height; i++)
	{
		MZScan* line = scans[i];
		while (line->getLcTime() > lcbuckets[y])
			y++;

		int y2 = y;
		auto ypos = line->getLcTime();

		lcFloat yoffset = std::min(0.5f, (ypos - lastYpos));
		if (i < height - 1)
			yoffset = std::min(yoffset, scans[i + 1]->getLcTime() - ypos);
		lastYpos = ypos;



		lcFloat target = ypos + yoffset;

		while (target > lcbuckets[y2])
			y2++;


		if (y2 >= ysize)
			y2 = ysize - 1;
		if (y >= ysize)
		{
			std::cout << "error sizey " << y << " >= " << ysize << "\n";
			std::cout << "error sizey " << line->getLcTime() << " >= " << info.lcRange.max << "\n";

			//	continue;
			y = ysize - 1;
		}

		size_t width = line->getIntensity().size();


		auto lineMZ = line->getMz();
		if (type == MZDataType::square)
			lineMZ = scans[0]->getMz();

		auto lineData = line->getIntensity();

		//here we wactually put the datainto buckets
		int x = 0;
		int cnt = 0;
		mzFloat lastMz = 0;
		for (int j = 0; j < width; j++)
		{
			while (lineMZ[j] > mzbuckets[x])
				x++;

			int x2 = x;

			//how far right to 'creep' the data (to avoid blanks being infilled)
			// either close to the gap from the previous data point, .25 dalton, or up to the next datapoint)
			mzFloat offset = std::min((mzFloat).15f, 2 * (lineMZ[j] - lastMz));



			if (j < width - 1)
				offset = std::min(offset, lineMZ[j + 1] - lineMZ[j]);



			mzFloat target = lineMZ[j] + offset;
			lastMz = lineMZ[j];

			while (target > mzbuckets[x2])
				x2++;

			auto value = lineData[j];
			sum += value;


			if (x2 >= xsize)
				x2 = xsize - 1;


			for (int yp = y; yp <= y2; yp++)
				for (int xp = x; xp <= x2; xp++)
				{
#if reduceType == 1
					data[yp * xsize + xp] = std::max(data[yp * xsize + xp], value);


#else
					if (xp > xsize)
						std::cout << "xerror size " << xp << "  " << xsize << " \n";

					data[yp * xsize + xp] += value;
					counts[yp * xsize + xp] ++;

#endif
				}
		}
		//		std::cout << "y=" << y << "  " <<cnt <<" \n ";

	}
#if reduceType == 1

#else
	//normalise (so bucketed values can reasonably be compared with different size buckets)
	if (reduceType == 0)
		for (int i = 0; i < xsize + 1; i++)
			for (int j = 0; j < ysize + 1; j++)
				if (counts[j * xsize + i] > 0)
					data[j * xsize + i] /= counts[j * xsize + i];
#endif

	MZData* newData = new MZData();
	newData->setJagged(false);

	std::vector<mzFloat>mzVals(xsize);
	for (int i = 0; i < xsize; i++)
		mzVals[i] = mzbuckets[i];

	for (int i = 0; i < ysize; i++)
	{
		lcFloat lcTime = lcbuckets[i];
		std::vector<signalFloat>intensityVals(xsize);
		for (int j = 0; j < xsize; j++)
			intensityVals[j] = data[i * xsize + j];
		MZScan* scan;
		if (i == 0)
			scan = new MZScan(mzVals, intensityVals, lcTime, newData);
		else
			scan = new MZScan(intensityVals, lcTime, newData);

		newData->append(scan);
	}

	if (newData->size() == 0)
		return NULL;

	newData->setRange();

	newData->info.lcRange = info.lcRange;
	newData->info.mzRange = info.mzRange;
	newData->info.signalRange = info.signalRange;

	return newData;
}


void MZData::copyRange(MZData* source) {
	info.lcRange = source->info.lcRange;
	info.mzRange = source->info.mzRange;
	info.signalRange = source->info.signalRange;

}





MZData::~MZData()
{
	clear();
	numMZData--;






}


void MZData::clear()
{
	times_cleared++;
	if (times_cleared > 1)
	{
		std::cout << "cleared again " << times_cleared << "  " << scans.size() << "\n";
	}

	for (auto scan : scans)
	{

		delete scan;
	}
	std::vector<MZScan*> tempvector;
	scans.swap(tempvector);
	if (scans.size() > 0)
	{
		std::cout << " clear error \n";
		scans.clear();
	}
}
std::vector<MZData*> MZData::split(int xsize, int ysize)
{
	return new_split(xsize, ysize);
}
