#!/bin/bash

echo "=== 库存与订单管理系统测试脚本 ==="
echo ""

# 测试步骤1: 创建商品
echo "1. 创建商品..."
curl -X POST http://localhost:8080/products \
  -H "Content-Type: application/json" \
  -d '{"name":"Apple iPhone 15","sku":"IP15-128-BLK","price":6999.0,"initial_stock":100,"reorder_threshold":10}'

echo "\n"

# 测试步骤2: 查询商品列表
echo "2. 查询商品列表..."
curl -X GET http://localhost:8080/products

echo "\n"

# 测试步骤3: 创建第二个商品
echo "3. 创建第二个商品..."
curl -X POST http://localhost:8080/products \
  -H "Content-Type: application/json" \
  -d '{"name":"AirPods Pro 2","sku":"APP2-GEN","price":1899.0,"initial_stock":50,"reorder_threshold":5}'

echo "\n"

# 测试步骤4: 创建订单
echo "4. 创建订单..."
curl -X POST http://localhost:8080/orders \
  -H "Content-Type: application/json" \
  -d '{"items":[{"product_id":1,"quantity":2},{"product_id":2,"quantity":1}]}'

echo "\n"

# 测试步骤5: 查询订单详情
echo "5. 查询订单详情..."
curl -X GET http://localhost:8080/orders/1

echo "\n"

# 测试步骤6: 更新订单状态为已支付
echo "6. 更新订单状态为已支付..."
curl -X POST http://localhost:8080/orders/1/status \
  -H "Content-Type: application/json" \
  -d '{"status":"PAID"}'

echo "\n"

# 测试步骤7: 再次查询订单详情
echo "7. 再次查询订单详情..."
curl -X GET http://localhost:8080/orders/1

echo "\n"

# 测试步骤8: 查询低库存商品
echo "8. 查询低库存商品..."
curl -X GET http://localhost:8080/stats/low_stock

echo "\n"

# 测试步骤9: 调整库存
echo "9. 调整商品库存..."
curl -X POST http://localhost:8080/products/1/adjust_stock \
  -H "Content-Type: application/json" \
  -d '{"delta":-5,"reason":"manual_correction"}'

echo "\n"

# 测试步骤10: 创建第二个订单（测试库存不足场景）
echo "10. 创建第二个订单（测试大量购买）..."
curl -X POST http://localhost:8080/orders \
  -H "Content-Type: application/json" \
  -d '{"items":[{"product_id":1,"quantity":200}]}'

echo "\n"

# 测试步骤11: 创建第三个订单（用于取消测试）
echo "11. 创建第三个订单（用于取消测试）..."
curl -X POST http://localhost:8080/orders \
  -H "Content-Type: application/json" \
  -d '{"items":[{"product_id":2,"quantity":2}]}'

echo "\n"

# 测试步骤12: 取消订单并回滚库存
echo "12. 取消订单并回滚库存..."
curl -X POST http://localhost:8080/orders/3/status \
  -H "Content-Type: application/json" \
  -d '{"status":"CANCELLED","restock":true}'

echo "\n"

# 测试步骤13: 获取今日统计数据
echo "13. 获取今日统计数据..."
TODAY=$(date +"%Y-%m-%d")
curl -X GET "http://localhost:8080/stats/daily_summary?date=$TODAY"

echo "\n"
echo "=== 测试完成 ==="
