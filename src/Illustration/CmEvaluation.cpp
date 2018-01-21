//#include "StdAfx.h"
#include "..\CmLib.h"
#include "CmEvaluation.h"

int CmEvaluation::STEP = 1;
const int CmEvaluation::NUM_THRESHOLD =  COLOR_NUM / STEP + 1;

void CmEvaluation::Evaluate(CStr gtW, CStr &salDir, CStr &resName, vecS &des)
{
	int NumMethod = des.size(); // Number of different methods
	vector<vecD> precision(NumMethod), recall(NumMethod), tpr(NumMethod), fpr(NumMethod);
	static const int CN = 21; // Color Number 
	static const char* c[CN] = {"'k'", "'b'", "'g'", "'r'", "'c'", "'m'", "'y'",
		"':k'", "':b'", "':g'", "':r'", "':c'", "':m'", "':y'", 
		"'--k'", "'--b'", "'--g'", "'--r'", "'--c'", "'--m'", "'--y'"
	};
	FILE* f = fopen(_S(resName), "w");
	CV_Assert(f != NULL);
	fprintf(f, "clear;\nclose all;\nclc;\n\n\n%%%%\nfigure(1);\nhold on;\n");
	vecD thr(NUM_THRESHOLD);
	for (int i = 0; i < NUM_THRESHOLD; i++)
		thr[i] = i * STEP;
	PrintVector(f, thr, "Threshold");
	fprintf(f, "\n");
	
	vecD mae(NumMethod);
	for (int i = 0; i < NumMethod; i++)
		mae[i] = Evaluate_(gtW, salDir, "_" + des[i] + ".png", precision[i], recall[i], tpr[i], fpr[i]); //Evaluate(salDir + "*" + des[i] + ".png", gtW, val[i], recall[i], t);

	string leglendStr("legend(");
	vecS strPre(NumMethod), strRecall(NumMethod), strTpr(NumMethod), strFpr(NumMethod);
	for (int i = 0; i < NumMethod; i++){
		strPre[i] = format("Precision_%s", _S(des[i]));
		strRecall[i] = format("Recall_%s", _S(des[i]));
		strTpr[i] = format("TPR_%s", _S(des[i]));
		strFpr[i] = format("FPR_%s", _S(des[i]));
		PrintVector(f, recall[i], strRecall[i]);
		PrintVector(f, precision[i], strPre[i]);
		PrintVector(f, tpr[i], strTpr[i]);
		PrintVector(f, fpr[i], strFpr[i]);
		fprintf(f, "plot(%s, %s, %s, 'linewidth', %d);\n", _S(strRecall[i]), _S(strPre[i]), c[i % CN], i < CN ? 2 : 1);
		leglendStr += format("'%s', ",  _S(des[i]));
	}
	leglendStr.resize(leglendStr.size() - 2);
	leglendStr += ");";
	string xLabel = "label('Recall');\n";
	string yLabel = "label('Precision')\n";
	fprintf(f, "hold off;\nx%sy%s\n%s\ngrid on;\naxis([0 1 0 1]);\ntitle('Precision recall curve');\n", _S(xLabel), _S(yLabel), _S(leglendStr));


	fprintf(f, "\n\n\n%%%%\nfigure(2);\nhold on;\n");
	for (int i = 0; i < NumMethod; i++)
		fprintf(f, "plot(%s, %s,  %s, 'linewidth', %d);\n", _S(strFpr[i]), _S(strTpr[i]), c[i % CN], i < CN ? 2 : 1);
	xLabel = "label('False positive rate');\n";
	yLabel = "label('True positive rate')\n";
	fprintf(f, "hold off;\nx%sy%s\n%s\ngrid on;\naxis([0 1 0 1]);\n\n\n%%%%\nfigure(3);\ntitle('ROC curve');\n", _S(xLabel), _S(yLabel), _S(leglendStr));

	double betaSqr = 0.3; // As suggested by most papers for salient object detection
	vecD areaROC(NumMethod, 0), avgFMeasure(NumMethod, 0), maxFMeasure(NumMethod, 0);
	for (int i = 0; i < NumMethod; i++){
		CV_Assert(fpr[i].size() == tpr[i].size() && precision[i].size() == recall[i].size() && fpr[i].size() == precision[i].size());
		for (size_t t = 0; t < fpr[i].size(); t++){
			double fMeasure = (1+betaSqr) * precision[i][t] * recall[i][t] / (betaSqr * precision[i][t] + recall[i][t]);
			avgFMeasure[i] += fMeasure/fpr[i].size(); // Doing average like this might have strange effect as in: 
			maxFMeasure[i] = max(maxFMeasure[i], fMeasure);
			if (t > 0){
				areaROC[i] += (tpr[i][t] + tpr[i][t - 1]) * (fpr[i][t - 1] - fpr[i][t]) / 2.0;

			}
		}
		fprintf(f, "%%%5s: AUC = %5.3f, MeanF = %5.3f, MaxF = %5.3f, MAE = %5.3f\n", _S(des[i]), areaROC[i], avgFMeasure[i], maxFMeasure[i], mae[i]);
	}
	PrintVector(f, areaROC, "AUC");
	PrintVector(f, avgFMeasure, "MeanFMeasure");
	PrintVector(f, maxFMeasure, "MaxFMeasure");
	PrintVector(f, mae, "MAE");

	// methodLabels = {'AC', 'SR', 'DRFI', 'GU', 'GB'};
	fprintf(f, "methodLabels = {'%s'", _S(des[0]));
	for (int i = 1; i < NumMethod; i++)
		fprintf(f, ", '%s'", _S(des[i]));
	fprintf(f, "};\n\nbar([MeanFMeasure; MaxFMeasure; AUC]');\nlegend('Mean F_\\beta', 'Max F_\\beta', 'AUC');xlim([0 %d]);\ngrid on;\n", NumMethod+1);
	fprintf(f, "xticklabel_rotate([1:%d],90, methodLabels,'interpreter','none');\n", NumMethod);
	fprintf(f, "\n\nfigure(4);\nbar(MAE);\ntitle('MAE');\ngrid on;\nxlim([0 %d]);", NumMethod+1);
	fprintf(f, "xticklabel_rotate([1:%d],90, methodLabels,'interpreter','none');\n", NumMethod);
	fclose(f);
	printf("%-70s\r", "");
}

void CmEvaluation::PrintVector(FILE *f, const vecD &v, CStr &name)
{
	fprintf(f, "%s = [", name.c_str());
	for (size_t i = 0; i < v.size(); i++)
		fprintf(f, "%g ", v[i]);
	fprintf(f, "];\n");
}

// Return mean absolute error (MAE)
double CmEvaluation::Evaluate_(CStr &gtImgW, CStr &inDir, CStr& resExt, vecD &precision, vecD &recall, vecD &tpr, vecD &fpr)
{
	vecS names;
	string truthDir, gtExt;
	int imgNum = CmFile::GetNamesNE(gtImgW, names, truthDir, gtExt); 
	precision.resize(NUM_THRESHOLD, 0);
	recall.resize(NUM_THRESHOLD, 0);
	tpr.resize(NUM_THRESHOLD, 0);
	fpr.resize(NUM_THRESHOLD, 0);
	if (imgNum == 0){
		printf("Can't load ground truth images %s\n", _S(gtImgW));
		return 10^20;
	}
	else
		printf("Evaluating %d saliency maps ... \r", imgNum);

	double mea = 0;
	for (int i = 0; i < imgNum; i++){
		if(i % 100 == 0)
			printf("Evaluating %03d/%d %-40s\r", i, imgNum, _S(names[i] + resExt));
		Mat resS = imread(inDir + names[i] + resExt, CV_LOAD_IMAGE_GRAYSCALE);
		CV_Assert_(resS.data != NULL, ("Can't load saliency map: %s\n", _S(names[i] + resExt)));
		normalize(resS, resS, 0, 255, NORM_MINMAX);
		Mat gtFM = imread(truthDir + names[i] + gtExt, CV_LOAD_IMAGE_GRAYSCALE), gtBM;
		if (gtFM.data == NULL) 
			continue;
		CV_Assert_(resS.size() == gtFM.size(), ("Saliency map and ground truth image size mismatch: %s\n", _S(names[i])));
		compare(gtFM, 128, gtFM, CMP_GT);
		bitwise_not(gtFM, gtBM);
		double gtF = sum(gtFM).val[0];
		double gtB = resS.cols * resS.rows * 255 - gtF;


#pragma omp parallel for
		for (int thr = 0; thr < NUM_THRESHOLD; thr++){
			Mat resM, tpM, fpM;
			compare(resS, thr * STEP, resM, CMP_GE);
			bitwise_and(resM, gtFM, tpM);
			bitwise_and(resM, gtBM, fpM);
			double tp = sum(tpM).val[0]; 
			double fp = sum(fpM).val[0];
			//double fn = gtF - tp;
			//double tn = gtB - fp;

			recall[thr] += tp/(gtF+EPS);
			double total = EPS + tp + fp;
			precision[thr] += (tp+EPS)/total;

			tpr[thr] += (tp + EPS) / (gtF + EPS);
			fpr[thr] += (fp + EPS) / (gtB + EPS);
		}

		gtFM.convertTo(gtFM, CV_32F, 1.0/255);
		resS.convertTo(resS, CV_32F, 1.0/255);
		cv::absdiff(gtFM, resS, resS);
		mea += sum(resS).val[0] / (gtFM.rows * gtFM.cols);
	}

	int thrS = 0, thrE = NUM_THRESHOLD, thrD = 1;
	for (int thr = thrS; thr != thrE; thr += thrD){
		precision[thr] /= imgNum;
		recall[thr] /= imgNum;
		tpr[thr] /= imgNum;
		fpr[thr] /= imgNum;
	}
	return mea/imgNum;
}


double CmEvaluation::interUnionBBox(const Vec4i &box1, const Vec4i &box2) // each box minx, minY, maxX, maxY
{
	int bi[4];
	bi[0] = max(box1[0], box2[0]);
	bi[1] = max(box1[1], box2[1]);
	bi[2] = min(box1[2], box2[2]);
	bi[3] = min(box1[3], box2[3]);	

	double iw = bi[2] - bi[0] + 1;
	double ih = bi[3] - bi[1] + 1;
	double ov = 0;
	if (iw>0 && ih>0){
		double ua = (box1[2]-box1[0]+1)*(box1[3]-box1[1]+1)+(box2[2]-box2[0]+1)*(box2[3]-box2[1]+1)-iw*ih;
		ov = iw*ih/ua;
	}	
	return ov;
}

void CmEvaluation::EvalueMask(CStr gtW, CStr &maskDir, CStr &des, CStr resFile, double betaSqr, bool alertNul, CStr suffix) 
{
	vecS descri(1); descri[0] = des; 
	EvalueMask(gtW, maskDir, descri, resFile, betaSqr, alertNul, suffix);
}

void CmEvaluation::EvalueMask(CStr gtW, CStr &maskDir, vecS &des, CStr resFile, double betaSqr, bool alertNul, CStr suffix, CStr title)
{
	vecS namesNS; 
	string gtDir, gtExt;
	int imgNum = CmFile::GetNamesNE(gtW, namesNS, gtDir, gtExt);
	int methodNum = (int)des.size();
	vecD pr(methodNum), rec(methodNum), count(methodNum), fm(methodNum);
	vecD intUnio(methodNum), mae(methodNum);
	for (int i = 0; i < imgNum; i++){
		Mat truM = imread(gtDir + namesNS[i] + gtExt, CV_LOAD_IMAGE_GRAYSCALE);
		for (int m = 0; m < methodNum; m++)	{
			string mapName = maskDir + namesNS[i] + "_" + des[m];
			mapName += suffix.empty() ? ".png" : "_" + suffix + ".png";
			Mat res = imread(mapName, CV_LOAD_IMAGE_GRAYSCALE);
			if (truM.data == NULL || res.data == NULL || truM.size != res.size){
				if (alertNul)
					printf("Truth(%d, %d), Res(%d, %d): %s\n", truM.cols, truM.rows, res.cols, res.rows, _S(mapName));
				continue;
			}
			compare(truM, 128, truM, CMP_GE);
			compare(res, 128, res, CMP_GE);
			Mat commMat, unionMat, diff1f;
			bitwise_and(truM, res, commMat);
			bitwise_or(truM, res, unionMat);
			double commV = sum(commMat).val[0];
			double p = commV/(sum(res).val[0] + EPS);
			double r = commV/(sum(truM).val[0] + EPS);
			pr[m] += p;
			rec[m] += r;
			intUnio[m] += commV / (sum(unionMat).val[0] + EPS);
			absdiff(truM, res, diff1f);
			mae[m] += sum(diff1f).val[0]/(diff1f.rows * diff1f.cols * 255);
			count[m]++;
		}
	}

	for (int m = 0; m < methodNum; m++){
		pr[m] /= count[m], rec[m] /= count[m];
		fm[m] = (1 + betaSqr) * pr[m] * rec[m] / (betaSqr * pr[m] + rec[m] + EPS);
		intUnio[m] /= count[m];
		mae[m] /= count[m];
	}

	FILE *f; 
	fopen_s(&f, _S(resFile), "a");
	if (f != NULL){
		fprintf(f, "\n%%%%\n");
		CmEvaluation::PrintVector(f, pr, "PrecisionMask" + suffix);
		CmEvaluation::PrintVector(f, rec, "RecallMask" + suffix);
		CmEvaluation::PrintVector(f, fm, "FMeasureMask" + suffix);
		CmEvaluation::PrintVector(f, intUnio, "IntUnion" + suffix);
		CmEvaluation::PrintVector(f, intUnio, "MAE" + suffix);
		fprintf(f, "bar([%s]');\ngrid on\n", _S("PrecisionMask" + suffix + "; RecallMask" + suffix + "; FMeasureMask" + suffix + "; IntUnion" + suffix));
		fprintf(f, "title('%s');\naxis([0 %d 0.8 1]);\nmethodLabels = { '%s'", _S(title), des.size() + 1, _S(des[0]));
		for (size_t i = 1; i < des.size(); i++)
			fprintf(f, ", '%s'", _S(des[i]));
		fprintf(f, " };\nlegend('Precision', 'Recall', 'FMeasure', 'IntUnion');\n");
		fprintf(f, "xticklabel_rotate([1:%d], 90, methodLabels, 'interpreter', 'none');\n", des.size());
		fclose(f);
	}

	if (des.size() == 1)
		printf("Precision = %g, recall = %g, F-Measure = %g, intUnion = %g, mae = %g\n", pr[0], rec[0], fm[0], intUnio[0], mae[0]);
}

double CmEvaluation::FMeasure(CMat &res, CMat &truM)
{
	Mat commMat;
	bitwise_and(truM, res, commMat);
	double commV = sum(commMat).val[0];
	double p = commV/(sum(res).val[0] + EPS);
	double r = commV/(sum(truM).val[0] + EPS);
	return 1.3*p*r / (0.3 *p + r);
}

void CmEvaluation::MeanAbsoluteError(CStr inDir, CStr salDir, vecS des, bool zeroMapIfMissing)
{
	vecS namesNE;
	int imgNum = CmFile::GetNamesNE(inDir + "*.jpg", namesNE), count = 0, methodNum = des.size();
	vecD costs(des.size());
	for (int i = 0; i < imgNum; i++){
		Mat gt = imread(inDir + namesNE[i] + ".png", CV_LOAD_IMAGE_GRAYSCALE);
		if (gt.empty())	{
			if (zeroMapIfMissing){
				Mat img = imread(inDir + namesNE[i] + ".jpg");
				gt = Mat::zeros(img.size(), CV_8U);
			}
			else 
				continue; 
		}		

		//printf("%s.jpg ", _S(namesNE[i]));
		gt.convertTo(gt, CV_32F, 1.0/255);
#pragma omp parallel for
		for (int j = 0; j < methodNum; j++){
			Mat res = imread(salDir + namesNE[i] + "_" + des[j] + ".png", CV_LOAD_IMAGE_GRAYSCALE);
			CV_Assert_(res.data != NULL, ("Can't load file %s\n", _S(namesNE[i] + "_" + des[j] + ".png")));
			if (res.size != gt.size){
				printf("size don't match %s\n", _S(namesNE[i] + "_" + des[j] + ".png"));
				resize(res, res, gt.size());
				CmFile::MkDir(salDir + "Out/");
				imwrite(salDir + "Out/" + namesNE[i] + "_" + des[j] + ".png", res);
			}
			res.convertTo(res, CV_32F, 1.0/255);
			cv::absdiff(gt, res, res);
			costs[j] += sum(res).val[0] / (gt.rows * gt.cols);
		}
		count++;
	}

	for (size_t j = 0; j < des.size(); j++)
		printf("%4s:%5.3f ", _S(des[j]), costs[j]/count);
	printf("\n");
}
