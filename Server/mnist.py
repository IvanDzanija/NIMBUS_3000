from pathlib import Path

import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import DataLoader
from torchvision import datasets, transforms


BASE_DIR = Path(__file__).resolve().parent
DATA_DIR = BASE_DIR / "data"
MODEL_PATH = BASE_DIR / "mnist_cnn.pt"

device = torch.device("mps" if torch.backends.mps.is_available() else "cpu")

train_transform = transforms.Compose(
    [
        transforms.RandomRotation(15),
        transforms.RandomAffine(0, translate=(0.1, 0.1)),
        transforms.ToTensor(),
    ]
)

test_transform = transforms.Compose([transforms.ToTensor()])


class MNISTCNN(nn.Module):
    def __init__(self) -> None:
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

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        return self.fc(self.conv(x))


def load_data() -> tuple[DataLoader, DataLoader, int]:
    train_dataset = datasets.MNIST(
        DATA_DIR, train=True, download=True, transform=train_transform
    )
    test_dataset = datasets.MNIST(
        DATA_DIR, train=False, download=True, transform=test_transform
    )

    train_loader = DataLoader(train_dataset, batch_size=64, shuffle=True)
    test_loader = DataLoader(test_dataset, batch_size=1000)
    return train_loader, test_loader, len(test_dataset)


def train(model: MNISTCNN, train_loader: DataLoader, optimizer, criterion) -> None:
    model.train()
    for data, target in train_loader:
        data, target = data.to(device), target.to(device)

        optimizer.zero_grad()
        output = model(data)
        loss = criterion(output, target)
        loss.backward()
        optimizer.step()


def test(model: MNISTCNN, test_loader: DataLoader, test_count: int) -> None:
    model.eval()
    correct = 0

    with torch.no_grad():
        for data, target in test_loader:
            data, target = data.to(device), target.to(device)
            pred = model(data).argmax(dim=1)
            correct += pred.eq(target).sum().item()

    acc = 100.0 * correct / test_count
    print(f"Accuracy: {acc:.2f}%")


def main() -> None:
    print("Using device:", device)
    train_loader, test_loader, test_count = load_data()

    model = MNISTCNN().to(device)
    criterion = nn.CrossEntropyLoss()
    optimizer = optim.Adam(model.parameters(), lr=0.001)

    for _ in range(5):
        train(model, train_loader, optimizer, criterion)
        test(model, test_loader, test_count)

    torch.save(model.state_dict(), MODEL_PATH)
    print(f"Saved model to {MODEL_PATH}")


if __name__ == "__main__":
    main()
