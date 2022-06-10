#pragma once

#include "Eigen/SparseCore"
#include "Eigen/SparseCholesky"

#include <vector>
#include <map>

class linearSystem {
    std::vector< std::map< unsigned int , double > > _ASparse;

    Eigen::SparseMatrix<double> _A , _At;
    Eigen::SimplicialLDLT< Eigen::SparseMatrix<double> > _AtA_choleskyDecomposition;

    std::vector< double > _b;

    unsigned int _rows , _columns;

public:
    linearSystem() {
        _ASparse.clear();
        setDimensions(0 , 0);
    }
    linearSystem( int rows , int columns ) {
        _ASparse.clear();
        setDimensions(rows , columns);
    }
    ~linearSystem() {
    }

    void setDimensions( int rows , int columns ) {
        _rows = rows; _columns = columns;
        _ASparse.resize(_rows);
        _b.resize(_rows);
        // this allows pushing equations without knowing in advance how much you have
    }

    double & A(unsigned int row , unsigned int column) {
        return _ASparse[row][column];
    }

    double & b(unsigned int row) {
        return _b[ row ];
    }

    void preprocess() {
        // convert ad-hoc matrix to Eigen sparse format:
        {
            _A.resize(_rows , _columns);
            std::vector< Eigen::Triplet< double > > triplets;
            for( unsigned int r = 0 ; r < _rows ; ++r ) {
                for( std::map< unsigned int , double >::const_iterator it = _ASparse[r].begin() ; it != _ASparse[r].end() ; ++it ) {
                    unsigned int c = it->first;
                    double val = it->second;
                    triplets.push_back( Eigen::Triplet< double >(r,c,val) );
                }
            }
            _A.setFromTriplets( triplets.begin() , triplets.end() );
        }
        _At = _A.transpose();
        Eigen::SparseMatrix<double> const & leftMatrix = _At * _A;
        _AtA_choleskyDecomposition.analyzePattern(leftMatrix);
        _AtA_choleskyDecomposition.compute(leftMatrix);
    }

    void solve( Eigen::VectorXd & X ) {

        Eigen::VectorXd _b_eigen( _b.size() );
        for( unsigned int i = 0 ; i < _b.size() ; ++i )
            _b_eigen[i] = _b[i];

        X = _AtA_choleskyDecomposition.solve( _At * _b_eigen );
    }
};
