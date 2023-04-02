import './App.css';
import salade from './salade.webp'

function App() {

  const product = {
    name: 'Salade césarienne',
    picture: salade,
    price: '5,00 €'
  }

  return (
    <div className="App">
      <header className="App-header">
        <div>
          <div>
            {product.name}
          </div>
          <div>
            <img src={product.picture} alt={product.name} />
          </div>
          <div>
            {product.price}
          </div>
        </div>
      </header>
    </div>
  );
}

export default App;
