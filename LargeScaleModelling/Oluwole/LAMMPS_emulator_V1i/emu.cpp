//to compile g++ -I/usr/include/eigen3/ emu.cpp -o emu
#include <iostream>
#include <fstream>
#include <Eigen/Dense>
#include <Eigen/Core>
using namespace std;
using namespace Eigen;

//this function computes distance correlation
MatrixXd mycor(const MatrixXd& X1,const MatrixXd& X2,const VectorXd& betai)
{
MatrixXd ones = MatrixXd::Ones(1,X2.rows());
MatrixXd ones2 = MatrixXd::Ones(1,X1.rows());
MatrixXd pdf = betai.asDiagonal();//turn vector to digaonal matrix
MatrixXd R1 = X1*pdf*X1.transpose();
MatrixXd R2 = X2*pdf*X2.transpose();
MatrixXd S1 = R1.diagonal()*ones;
MatrixXd S2 = R2.diagonal()*ones2;
MatrixXd a1 = X1*pdf*X2.transpose(); //t(tx1)%*%pdf%*%tx2
MatrixXd a2 = X2*pdf*X1.transpose();
MatrixXd res = (a1.transpose()+a2-S1.transpose()-S2);
ArrayXXd res2 = res.array();
return(res2.exp());
}
///////////////////////
//main function
int main()
{
	int nrows=2900;
	int ncols=9;  //number of outputs change from 9 to 14
	int ncols2=9;
	double tau0=1;  //modify
	double tau=0.0001000;
VectorXd betai(9);
betai << 3.4938999,3.4186248,3.8043138,0.8883166,1.1512325,2.9979449,2.5629414,0.8038899,1.1486920;

//read training input dat matrix X
MatrixXd X = MatrixXd::Zero(nrows,ncols2);
ifstream fin1 ("./X.txt");
if (fin1.is_open())
{
for (int row = 0; row < nrows; row++)
for (int col = 0; col < ncols2; col++)
{
float item;
fin1 >> item;
X(row, col) = item;
}
fin1.close();
}
//read in output matrix Y
MatrixXd Y = MatrixXd::Zero(nrows,ncols);
ifstream fin2 ("./Y.txt");
if (fin2.is_open())
{
for (int row = 0; row < nrows; row++)
for (int col = 0; col < ncols; col++)
{
float item;
fin2 >> item;
Y(row, col) = item;
}
fin2.close();
}

//read in new data point for prediction
MatrixXd newdata = MatrixXd::Zero(1,ncols2);
ifstream fin3 ("./input/input.txt");
if (fin3.is_open())
{
for (int row = 0; row < 1; row++)
for (int col = 0; col < ncols2; col++)
{
float item;
fin3 >> item;
newdata(row,col) = item;
}
fin3.close();
}

//read in Ainv matrix
MatrixXd Ainv = MatrixXd::Zero(nrows,nrows);
ifstream fin4 ("./Ainv2.txt"); //modify
if (fin4.is_open())
{
for (int row = 0; row < nrows; row++) //modify
for (int col = 0; col < nrows; col++) //modify
{
float item;
fin4 >> item;
Ainv(row, col) = item;
}
fin4.close();
}

//read in iOmega matrix
MatrixXd iOmega = MatrixXd::Zero(ncols2,ncols2);
ifstream fin5 ("./iOmega.txt");
if (fin5.is_open())
{
for (int row = 0; row < ncols2; row++)
for (int col = 0; col < ncols2; col++)
{
float item;
fin5 >> item;
iOmega(row, col) = item;
}
fin5.close();
}
//read in betahat matrix
MatrixXd betahat = MatrixXd::Zero(ncols2,ncols);
ifstream fin6 ("./betahat.txt");
if (fin6.is_open())
{
for (int row = 0; row < ncols2; row++)
for (int col = 0; col < ncols; col++)
{
float item;
fin6 >> item;
betahat(row, col) = item;
}
fin6.close();
}
//normalization data
MatrixXd s1(1,9); 
MatrixXd s2(1,9);  
s2 << 4.131475e-02,1.308324e-02,8.995006e-03,8.995000e-03,8.995000e-03,2.275674e-10,2.159337e-10,3.977501e-14,3.987490e-14; 
s1 << 3.593248e-03,1.885759e-03,2.622689e-119,5.293362e-118,4.234222e-116,1.080608e-14,4.539888e-15,2.840420e-15,2.840420e-15; 
VectorXd s3(9);
s3 << -6.245067,
 -6.39793, 
 -6.948577,
  -8.282121,
   -5.484406,
    -32.23061,
     4.382027,
      -13.79393,
       -15.26332;
MatrixXd H = X.array();// training inputs
MatrixXd H0 = (newdata.array()-s1.array())/s2.array();//scale the test input
int m=H.cols();
int n=Y.rows();
int n2=H0.rows();
MatrixXd A00, A01;

//MatrixXd gaga = MatrixXd::Zero(2900,9);
// gaga = (H.colwise()-s3);//.rowwise().squaredNorm();
A01.noalias() = mycor(H0,H,betai);//###cross correlation//dont evaluate yet
 A00.noalias() = mycor(H0,H0,betai);//##test point correlation
//MatrixXd A = mycor(H,H,betai);//##very big matrix precomputed in R
MatrixXd temp = MatrixXd::Identity(A00.rows(), A00.cols());
 A00 += ((tau0*tau)*temp);
//Ainv.noalias() = A.inverse();
 /**
//MatrixXd iOmega = (H.transpose()*Ainv*H).inverse();//can be precomputed in R
//MatrixXd betahat= iOmega*(H.transpose()*Ainv*Y);// can be precomputed in R
* */

MatrixXd mu_star = H0*betahat+A01.transpose()*Ainv.selfadjointView<Upper>()*(Y-H*betahat); 
MatrixXd r1 = H0-(A01.transpose()*Ainv.selfadjointView<Upper>()*H);
MatrixXd c_star = A00-(A01.transpose()*Ainv.selfadjointView<Upper>()*A01)+(((r1)*(iOmega)*r1.transpose()));
//MatrixXd Sigma = ((Y-H*betahat).transpose()*Ainv.selfadjointView<Upper>()*(Y-H*betahat))/(n-m); precomputed in R
//MatrixXd Sigma(4,4);
//Sigma << 3.273639283317, 33.2402996825996, 22.3515408957194, 7.52106455280949,
//33.2402996826023, 33.3135878103714, 22.3313232827605, 7.51532055692301,
//22.3515408957016, 22.3313232827417, 31.1848209695244, -5.26625223699458,
//7.52106455282121, 7.51532055693379, -5.26625223699611, 33.5773444066967;
MatrixXd rate = mu_star;//(mu_star.array()*s2.array())+s1.array();
//output results
ofstream a_file ("./output/Rate.txt");
a_file << rate;
a_file.close();
//cout << H0 << endl;
}