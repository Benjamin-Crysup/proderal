#include "whodun_align_merge.h"

#include <math.h>

AlignedSequenceMerger::AlignedSequenceMerger(){}
AlignedSequenceMerger::~AlignedSequenceMerger(){}

void AlignedSequenceMerger::changeFocus(uintptr_t seq1Len, const char* seq1, const double* qual1, uintptr_t seq2Len, const char* seq2, const double* qual2, LinearPairwiseAlignmentIteration* alnUse){
	saveSeqA.clear(); saveSeqA.insert(saveSeqA.end(), seq1, seq1 + seq1Len);
	saveQualA.clear(); saveQualA.insert(saveQualA.end(), qual1, qual1 + seq1Len);
	saveSeqB.clear(); saveSeqB.insert(saveSeqB.end(), seq2, seq2 + seq2Len);
	saveQualB.clear(); saveQualB.insert(saveQualB.end(), qual2, qual2 + seq2Len);
	curIter = alnUse;
	seqIL.clear();
	seqdIL.clear();
	seqIR.clear();
	seqdIR.clear();
	mergeSeq.clear();
	mergeQual.clear();
}

void AlignedSequenceMerger::changeFocus(uintptr_t seq1Len, const char* seq1, const char* qual1, uintptr_t seq2Len, const char* seq2, const char* qual2, LinearPairwiseAlignmentIteration* alnUse){
	saveSeqA.clear(); saveSeqA.insert(saveSeqA.end(), seq1, seq1 + seq1Len);
	saveQualA.resize(seq1Len); fastaPhredsToLog10Prob(seq1Len, (const unsigned char*)qual1, &(saveQualA[0]));
	saveSeqB.clear(); saveSeqB.insert(saveSeqB.end(), seq2, seq2 + seq2Len);
	saveQualB.resize(seq2Len); fastaPhredsToLog10Prob(seq2Len, (const unsigned char*)qual2, &(saveQualB[0]));
	curIter = alnUse;
	seqIL.clear();
	seqdIL.clear();
	seqIR.clear();
	seqdIR.clear();
	mergeSeq.clear();
	mergeQual.clear();
}

/**
 * Fill in quality for indels.
 * @param forSeq The sequence to fill quality for: -1 means indel.
 * @param forQual The quality to update.
 */
void AlignedSequenceMerger_fillInIndelQuality(std::vector<int>* forSeq, std::vector<double>* forQual){
	uintptr_t ai = 0;
	while(ai < forSeq->size()){
		if((*forSeq)[ai] >= 0){
			ai++; continue;
		}
		uintptr_t nai = ai + 1;
		while((nai < forSeq->size()) && ((*forSeq)[nai] < 0)){
			nai++;
		}
		double winQual;
		if(ai && (nai < forSeq->size())){
			double leftQ = (*forQual)[ai-1];
			double rightQ = (*forQual)[nai];
			winQual = log10((pow(10.0, leftQ) + pow(10.0, rightQ))/2.0);
		}
		else if(ai){ winQual = (*forQual)[ai-1]; }
		else if(nai < forSeq->size()){ winQual = (*forQual)[nai]; }
		else{ winQual = -1.0 / 0.0; }
		for(uintptr_t ci = ai; ci < nai; ci++){
			(*forQual)[ci] = winQual;
		}
		ai = nai;
	}
}

void AlignedSequenceMerger::expandIndels(){
	uintptr_t alnLen = curIter->aInds.size();
	//expand out indels
	seqIL.clear(); seqdIL.clear();
	seqIR.clear(); seqdIR.clear();
	for(uintptr_t i = 1; i<alnLen; i++){
		if(curIter->aInds[i] != curIter->aInds[i-1]){
			if(curIter->bInds[i] != curIter->bInds[i-1]){
				//consume both
				seqIL.push_back(saveSeqA[curIter->aInds[i-1]]);
				seqdIL.push_back(saveQualA[curIter->aInds[i-1]]);
				seqIR.push_back(saveSeqB[curIter->bInds[i-1]]);
				seqdIR.push_back(saveQualB[curIter->bInds[i-1]]);
			}
			else{
				//insert a, skip b
				seqIL.push_back(saveSeqA[curIter->aInds[i-1]]);
				seqdIL.push_back(saveQualA[curIter->aInds[i-1]]);
				seqIR.push_back(-1);
				seqdIR.push_back(0.0);
			}
		}
		else{
			//insert b, skip a
			seqIL.push_back(-1);
			seqdIL.push_back(0.0);
			seqIR.push_back(saveSeqB[curIter->bInds[i-1]]);
			seqdIR.push_back(saveQualB[curIter->bInds[i-1]]);
		}
	}
	//fill in any indel qualities
	AlignedSequenceMerger_fillInIndelQuality(&seqIL, &seqdIL);
	AlignedSequenceMerger_fillInIndelQuality(&seqIR, &seqdIR);
}

void AlignedSequenceMerger::addInPrefix(){
	if(curIter->aInds[0] != 0){
		if(curIter->bInds[0] != 0){
			throw std::runtime_error("No clear prefix to add.");
		}
		mergeSeq.insert(mergeSeq.end(), saveSeqA.begin(), saveSeqA.begin() + curIter->aInds[0]);
		mergeQual.insert(mergeQual.end(), saveQualA.begin(), saveQualA.begin() + curIter->aInds[0]);
	}
	else{
		mergeSeq.insert(mergeSeq.end(), saveSeqB.begin(), saveSeqB.begin() + curIter->bInds[0]);
		mergeQual.insert(mergeQual.end(), saveQualB.begin(), saveQualB.begin() + curIter->bInds[0]);
	}
}

void AlignedSequenceMerger::mergeOverlap(){
	for(uintptr_t i = 0; i<seqIL.size(); i++){
		int sac = seqIL[i];
		double lqac = seqdIL[i];
		int sbc = seqIR[i];
		double lqbc = seqdIR[i];
		double errA = pow(10.0, lqac);
		double errB = pow(10.0, lqbc);
		if(lqac < lqbc){
			if(sac >= 0){
				mergeSeq.push_back(sac);
				if(sac != sbc){
					double numVal = errA * (1.0 - errB/3.0);
					double denVal = errA + errB - 4*errA*errB/3.0;
					mergeQual.push_back(log10(numVal/denVal));
				}
				else{
					double numVal = errA*errB / 3;
					double denVal = 1 - errA - errB + 4*errA*errB/3.0;
					mergeQual.push_back(log10(numVal/denVal));
				}
			}
		}
		else{
			if(sbc >= 0){
				mergeSeq.push_back(sbc);
				if(sac != sbc){
					double numVal = errB * (1.0 - errA/3.0);
					double denVal = errA + errB - 4*errA*errB/3.0;
					mergeQual.push_back(log10(numVal/denVal));
				}
				else{
					double numVal = errA*errB / 3;
					double denVal = 1 - errA - errB + 4*errA*errB/3.0;
					mergeQual.push_back(log10(numVal/denVal));
				}
			}
		}
	}
}

void AlignedSequenceMerger::addInSuffix(){
	uintptr_t alnLen = curIter->aInds.size();
	if((uintptr_t)(curIter->aInds[alnLen-1]) < saveSeqA.size()){
		if((uintptr_t)(curIter->bInds[alnLen-1]) < saveSeqB.size()){
			throw std::runtime_error("No obvious suffix.");
		}
		mergeSeq.insert(mergeSeq.end(), saveSeqA.begin() + curIter->aInds[alnLen-1], saveSeqA.end());
		mergeQual.insert(mergeQual.end(), saveQualA.begin() + curIter->aInds[alnLen-1], saveQualA.end());
	}
	else{
		mergeSeq.insert(mergeSeq.end(), saveSeqB.begin() + curIter->bInds[alnLen-1], saveSeqB.end());
		mergeQual.insert(mergeQual.end(), saveQualB.begin() + curIter->bInds[alnLen-1], saveQualB.end());
	}
}



