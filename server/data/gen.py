import random
import time
import csv

# Generate fake data
def generate_data(n=300):
    data = []
    for _ in range(n):
        user_id = random.randint(1, 300)
        item_id = random.randint(1000, 1300)
        category_id = random.randint(1, 50)
        rating = round(random.uniform(1.0, 5.0), 1)
        timestamp = random.randint(915148800, 1009843200)  # Random timestamp between 1999 and 2002
        data.append([user_id, item_id, category_id, rating, timestamp])
    return data

# Generate and save
data = generate_data(500)

output_path = "./data/fake_reco_data.txt"
with open(output_path, "w", newline="") as f:
    writer = csv.writer(f, delimiter=';')
    writer.writerows(data)
