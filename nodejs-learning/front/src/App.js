import './App.css';
import salade from './salade.webp'

function App() {
  return (
    <div className="App">
      <header className="App-header">
        <div>
          <div>
            Salade césarienne
          </div>
          <div>
            <img src={salade} alt="Salade césarienne" />
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
