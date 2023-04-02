import './App.css';
import salade from './salade.webp'

function App() {

  const product = {
    name: 'Salade césarienne',
    picture: salade,
    pictureDescription: 'Salade césarienne',
  }

  return (
    <div className="App">
      <header className="App-header">
        <div>
          <div>
            {product.name}
          </div>
          <div>
            <img src={product.picture} alt={product.pictureDescription} />
          </div>
          <div>
            5,00 €
          </div>
        </div>
      </header>
    </div>
  );
}

export default App;
