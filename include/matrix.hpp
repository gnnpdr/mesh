#pragma once

#include "vec3.hpp"

namespace Matrix
{

template<size_t rows_, size_t cols_>
class Matrix
{
    float matrix_[rows_][cols_];

public:

    Matrix()
    {
        for (size_t r = 0; r < rows_; r++)
        {
            for (size_t c = 0; c < cols_; c++)
                matrix[r][c] = 0.0f;
        }
    }

    float& operator()(size_t r, size_t c)
    {
        if (r > rows_, c > cols_)
        {
            std::cout << "trouble, sir" << std::endl;
            return 0.0f;
        }

        return matrix[r][c];
    }

    float operator()(size_t r, size_t c) const
    {
        if (r > rows_, c > cols_)
        {
            std::cout << "trouble, sir" << std::endl;
            return 0.0f;
        }

        return matrix[r][c];
    }

    Matrix<rows_, cols_>& operator+(Matrix<rows_, cols_>& const other) const 
    {
        Matrix<rows_, cols_> sum();

        for (size_t r = 0; r < rows; r++)
        {
            for (size_t c = 0; c < cols_; c++)
                sum = matrix_[r][c] + other[r][c];
        }

        return sum;
    }

    Matrix<rows_, 1>& operator*(Matrix<rows_, 1>& vector) const
    {
        Matrix<rows_, 1> new_vector;

        for (size_t r = 0; r < rows_; r++)
        {
            float row_sum = 0.0f;
            for (size_t c = 0; c < cols_; c++)
                row_sum += matrix[r, c];

            new_vector[r][0] = vector[r][0] * row_sum;
        }

        return new_vector;
    }

    Matrix<rows_, cols_>& operator*(float scalar) const
    {
        Matrix<rows_, cols_> new_matrix;

        for (size_t r = 0; r < rows_; r++)
        {
            for (size_t c = 0; c < cols_; c++)
                new_matrix[r][c]*scalar;
        }

        return new_matrix;
    }

    Matrix<3, 3>& get_sub_A()
    {
        Matrix<3, 3> submatrix();
        if (rows_ >= 3 || cols_ >= 3)
        {
            std::cout << "too small, sir" << std::endl;
            return submatrix;
        }

        for (size_t r = 0; r < 3; r++)
        {
            for (size_t c = 0 ; c < 3; c++)
                submatrix = matrix_[r][c];
        }

        return submatrix;
    }

    Matrix<3, 1>& get_B()
    {
        Matrix<3, 1> B();

        for (size_t r = 0; r < 3; r++)
            B[r][0] = -matrix_[r][3];

        return B;
    } 

    void print()
    {
        for (size_t r = 0; r < rows_; r++)
        {
            for (size_t c = 0; c < cols_; c++)
                std::cout << matrix[r][c] << " ";

            std::cout << std::endl;
        }
    }
};

template<size_t rows_>
using Vector = Matrix<rows_, 1>;

}



namespace Gauss
{
class Solver
{
    Matrix::Matrix<3, 3>& A_;
    Matrix::Vector<3>& B_;
public:

    Solver(Matrix::Matrix<3, 3>& const A, Matrix::Vector<3>& const B) : A_(A), B_(B) {}

    explicit Solver(Matrix::Matrix<4, 4> Q) : A_(Q.get_sub_A()), B_(Q.get_B()) {}


    Matrix::Vector<3>& solve()
    {
        for (size_t c = 0; c < 3; c++)
        {
            size_t max_row = c;
            for (size_t r = c + 1; r < 3; r++)
            {
                if (std::abs(A_(r, c)) > std::abs(A_(max_row, c)))
                    max_row = r;
            }

            if (max_row != c) 
            {
                for (size_t col = 0; col < 3; col++)
                    std::swap(A_(c, col), A_(max_row, col));
        
                std::swap(B_(c, 0), B_(max_row, 0));
            }

            for (size_t row = c + 1; row < 3; row++) 
            {
                float factor = A_(row, c) / A_(c, c);
                for (size_t col = row; col < 3; col++)
                    A_(row, col) -= factor * A_(c, col);
            
                B_(row, 0) -= factor * B_(c, 0);
            }
        }

        Matrix::Vector<3> result;

        for (size_t row = 2; row >= 0; row--) 
        {
            result(row, 0) = B_(row, 0);
            for (int col = row + 1; col < 3; col++) 
                result(row, 0) -= A_(row, col) * result(col, 0);

            result(row, 0) /= A_(row, row);
        }

        return result;
    }

private:

};
}