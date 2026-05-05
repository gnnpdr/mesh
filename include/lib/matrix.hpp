#pragma once

#include "vec3.hpp"

#include<vector>

namespace Matrix
{

namespace Detail
{
    static float EPSILON = 1e-6;
}

/**
 * @brief Matrix with float components with basic mathematical operations
 * 
 * The class provides basic operations for working with matrix:
 * - Addition, subtraction
 * - Operations with a scalar
 * - Dot and vector products
 * - Normalization and length calculation
 * 
 * @tparam rows_ - amount of rows
 * @tparam cols_ - amount of columns
 */
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
                matrix_[r][c] = 0.0f;
        }
    }

    Matrix(std::vector<float> v)
    {
        size_t rows = v.size();

        for (size_t r = 0; r < rows; r++)
            matrix_[v[r]][0];
    }

    float& operator()(size_t r, size_t c)
    {
        if (r >= rows_ || c >= cols_)
            throw std::out_of_range("Matrix index out of range");
    
        return matrix_[r][c];
    }

    float operator()(size_t r, size_t c) const
    {
        if (r >= rows_ || c >= cols_)
            throw std::out_of_range("Matrix index out of range");

        return matrix_[r][c];
    }

    Matrix<rows_, cols_> operator+(const Matrix<rows_, cols_>& other) const 
    {
        Matrix<rows_, cols_> result;

        for (size_t r = 0; r < rows_; r++)
        {
            for (size_t c = 0; c < cols_; c++)
                result(r, c) = matrix_[r][c] + other(r, c);
        }

        return result;
    }

    template<size_t other_rows_, size_t other_cols_>
    Matrix<rows_, other_cols_> operator*(const Matrix<other_rows_, other_cols_>& other_matrix) const
    {
        Matrix<rows_, other_cols_> new_matrix;

        if (cols_ != other_rows_) 
            throw std::invalid_argument("Matrix multiplication dimension mismatch");

        for (size_t r = 0; r < rows_; r++)
        {
            for (size_t oc = 0; oc < other_cols_; oc++)
            {
                float new_el = 0.0f;
                for (size_t c = 0; c < cols_; c++)
                    new_el += matrix_[r][c] * other_matrix(c, oc);

                new_matrix(r, oc) = new_el;
            }
        }

        return new_matrix;
    }

    Matrix<rows_, cols_> operator*(float scalar) const
    {
        Matrix<rows_, cols_> new_matrix;

        for (size_t r = 0; r < rows_; r++)
        {
            for (size_t c = 0; c < cols_; c++)
                new_matrix(r, c) * scalar;
        }

        return new_matrix;
    }

    Matrix<3, 3> get_sub_A() const
    {
        Matrix<3, 3> submatrix;

        if (rows_ < 3 || cols_ < 4)
            throw std::invalid_argument("Matrix for getting A submatrix should be 3 x 4 minimum");

        for (size_t r = 0; r < 3; r++)
        {
            for (size_t c = 0 ; c < 3; c++)
                submatrix(r, c) = matrix_[r][c];
        }

        return submatrix;
    }

    Matrix<3, 1> get_B()
    {
        Matrix<3, 1> B;

        if (rows_ < 3)
            throw std::invalid_argument("Matrix for getting B submatrix should have 3 rows minimum");

        for (size_t r = 0; r < 3; r++)
            B(r, 0) = -matrix_[r][3];

        return B;
    } 

    Matrix<1, rows_> transpose()
    {
        if (cols_ != 1)
            throw std::invalid_argument("This func transposes just vectors");

        Matrix<1, rows_> t_vec;

        for (size_t r = 0; r < rows_; r++)
            t_vec(0, r) = matrix_[r][0];

        return t_vec;
    }

    void print()
    {
        for (size_t r = 0; r < rows_; r++)
        {
            for (size_t c = 0; c < cols_; c++)
                std::cout << matrix_(r, c) << " ";

            std::cout << std::endl;
        }
    }

    /**
     * @brief Сhecks by determinant whether a matrix is degenerated
     * 
     * @return true if degenerated
     */
    bool is_degenerate() const 
    {
        float det = determinant();
        return std::abs(det) < Detail::EPSILON;
    }

    float determinant() const 
    {
        float det = 0.0f;

        det += matrix_[0][0] * minor(0, 0);
        det -= matrix_[0][1] * minor(0, 1);
        det += matrix_[0][2] * minor(0, 2);
        det -= matrix_[0][3] * minor(0, 3);

        return det;
    }

    float minor(int row, int col) const 
    {
        float sub[3][3];
        int sub_i = 0, sub_j;

        for (int i = 0; i < 4; i++)
         {
            if (i == row) continue;

            sub_j = 0;
            for (int j = 0; j < 4; j++) 
            {
                if (j == col) continue;

                sub[sub_i][sub_j] = matrix_[i][j];
                sub_j++;
            }
            sub_i++;
        }

        return sub[0][0] * (sub[1][1] * sub[2][2] - sub[1][2] * sub[2][1])
             - sub[0][1] * (sub[1][0] * sub[2][2] - sub[1][2] * sub[2][0])
             + sub[0][2] * (sub[1][0] * sub[2][1] - sub[1][1] * sub[2][0]);
    }

    bool has_nan() const 
    {
        for (int i = 0; i < 4; i++) 
        {
            for (int j = 0; j < 4; j++) 
            {
                if (std::isnan(matrix_[i][j]) || std::isinf(matrix_[i][j]))
                    return true;
            }
        }
        return false;
    }
};

template<size_t rows_>
using Vector = Matrix<rows_, 1>;

using Quadric = Matrix<4, 4>;
}


/**
 * @brief Calcs matrix equetion using gauss method
 * 
 * 
 */
namespace Gauss
{
class Solver
{
    Matrix::Matrix<3, 3> A_;
    Matrix::Vector<3> B_;
public:

    Solver(const Matrix::Matrix<3, 3>& A, const Matrix::Vector<3>& B) : A_(A), B_(B) {}

    explicit Solver(Matrix::Matrix<4, 4> Q) : A_(Q.get_sub_A()), B_(Q.get_B()) {}


    Matrix::Vector<3> solve()
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

        for (int row = 2; row >= 0; row--) 
        {
            result(row, 0) = B_(row, 0);
            for (int col = row + 1; col < 3; col++) 
                result(row, 0) -= A_(row, col) * result(col, 0);

            result(row, 0) /= A_(row, row);
        }

        return result;
    }
};
}