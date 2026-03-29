import torch
import torch.nn as nn
import torch.optim as optim
from torchvision import datasets, transforms
from torch.utils.data import DataLoader

# ==============================
# Device (M2)
# ==============================
device = torch.device("mps" if torch.backends.mps.is_available() else "cpu")
print("Using device:", device)

# ==============================
# Transforms
# ==============================
train_transform = transforms.Compose(
    [
        transforms.RandomRotation(15),
        transforms.RandomAffine(0, translate=(0.1, 0.1)),
        transforms.ToTensor(),
    ]
)

test_transform = transforms.Compose(
    [
        transforms.ToTensor(),
    ]
)

# ==============================
# Dataset
# ==============================
train_dataset = datasets.MNIST(
    "./data", train=True, download=True, transform=train_transform
)
test_dataset = datasets.MNIST(
    "./data", train=False, download=True, transform=test_transform
)

train_loader = DataLoader(train_dataset, batch_size=64, shuffle=True)
test_loader = DataLoader(test_dataset, batch_size=1000)


# ==============================
# Model
# ==============================
class MNISTCNN(nn.Module):
    def __init__(self):
        super().__init__()
        self.conv = nn.Sequential(
            nn.Conv2d(1, 32, 3, padding=1),
            nn.ReLU(),
            nn.Conv2d(32, 64, 3, padding=1),
            nn.ReLU(),
            nn.MaxPool2d(2),
            nn.Conv2d(64, 128, 3, padding=1),
            nn.ReLU(),
            nn.MaxPool2d(2),
        )

        self.fc = nn.Sequential(
            nn.Flatten(),
            nn.Linear(128 * 7 * 7, 128),
            nn.ReLU(),
            nn.Dropout(0.3),
            nn.Linear(128, 10),
        )

    def forward(self, x):
        return self.fc(self.conv(x))


model = MNISTCNN().to(device)

# ==============================
# Training setup
# ==============================
criterion = nn.CrossEntropyLoss()
optimizer = optim.Adam(model.parameters(), lr=0.001)


# ==============================
# Train
# ==============================
def train():
    model.train()
    for data, target in train_loader:
        data, target = data.to(device), target.to(device)

        optimizer.zero_grad()
        output = model(data)
        loss = criterion(output, target)
        loss.backward()
        optimizer.step()


# ==============================
# Test
# ==============================
def test():
    model.eval()
    correct = 0

    with torch.no_grad():
        for data, target in test_loader:
            data, target = data.to(device), target.to(device)
            pred = model(data).argmax(dim=1)
            correct += pred.eq(target).sum().item()

    acc = 100.0 * correct / len(test_dataset)
    print(f"Accuracy: {acc:.2f}%")


# ==============================
# Run
# ==============================
for epoch in range(5):
    train()
    test()

# ==============================
# SAVE (FIXED)
# ==============================
torch.save(model.state_dict(), "mnist_cnn.pt")
print("Saved as mnist_cnn.pt")
