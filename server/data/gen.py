import random
import time
import csv

# Generate fake data
def generate_data(n=300):
    data = []
    # Unix timestamps:
    # January 1, 1970 00:00:00 UTC = 0
    # January 1, 2025 00:00:00 UTC = 1735689600
    start_timestamp = 0           # 1970-01-01
    end_timestamp = 1735689600    # 2025-01-01
    
    for _ in range(n):
        user_id = random.randint(1, 300)
        item_id = random.randint(1000, 1300)
        category_id = random.randint(1, 50)
        rating = round(random.uniform(1.0, 5.0), 1)
        timestamp = random.randint(start_timestamp, end_timestamp)
        data.append([user_id, item_id, category_id, rating, timestamp])
    return data

# Generate and save
data = generate_data(5000)

output_path = "./server/data/ratings.txt"
with open(output_path, "w", newline="") as f:
    writer = csv.writer(f, delimiter=';')
    writer.writerows(data)

print(f"Generated {len(data)} records with timestamps between 1970-2025")
print(f"Data saved to {output_path}")
