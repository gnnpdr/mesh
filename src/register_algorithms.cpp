/**
 * @file adapters.cpp
 * @brief Регистрация адаптеров алгоритмов упрощения в глобальном реестре
 */
#include "algorithms/adapters.hpp"
#include "algorithms/simplifier_registry.hpp"

namespace MeshSimplify 
{
/**
 * @brief Автоматическая регистрация алгоритмов
 * 
 * Макросы REGISTER_SIMPLIFIER создают статические объекты,
 * которые регистрируют алгоритмы в реестре до вызова main().
 * 
 * Для добавления нового алгоритма надо
 * Создать класс-адаптер, наследующий ISimplifier
 * Добавить соответствующий макрос: REGISTER_SIMPLIFIER(YourAdapter, "Display Name")
 */
REGISTER_SIMPLIFIER(VertexClusterAdapter, "Vertex Cluster")
REGISTER_SIMPLIFIER(EdgeCollapseAdapter, "Edge Collapse")
REGISTER_SIMPLIFIER(QuadricAdapter, "Quadric")
// REGISTER_SIMPLIFIER(MyCoolAlgorithmAdapter, "My Cool Algorithm")

}